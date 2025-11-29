const std = @import("std");
const c = @cImport({
    @cInclude("dlfcn.h");
});

// C function pointer types matching plugin contract
const InvokeFn = *const fn ([*c]const u8, [*c]const u8, [*c]const u8) callconv(.c) [*c]const u8;
const AttachFn = *const fn (InvokeFn, [*c]u8, usize) callconv(.c) bool;
const DetachFn = *const fn ([*c]u8, usize) callconv(.c) bool;

const LibsInfo = extern struct {
    plugin_type: [*c]const u8,
    product: [*c]const u8,
    description_long: [*c]const u8,
    description_short: [*c]const u8,
    plugin_id: u64,
};

const ReportFn = *const fn ([*c]u8, usize, *LibsInfo) callconv(.c) bool;

const ControlFns = struct {
    handle: *anyopaque,
    attach: AttachFn,
    detach: DetachFn,
    invoke: InvokeFn,
};

fn controlBoot(allocator: std.mem.Allocator) !?ControlFns {
    std.debug.print("HOST: Discovering control plugin...\n", .{});

    const exe_path = try std.fs.selfExePathAlloc(allocator);
    defer allocator.free(exe_path);

    const exe_dir = std.fs.path.dirname(exe_path) orelse ".";
    std.debug.print("HOST: Scanning directory: {s}\n", .{exe_dir});

    const lib_ext = if (@import("builtin").os.tag == .macos)
        ".dylib"
    else if (@import("builtin").os.tag == .windows)
        ".dll"
    else
        ".so";

    var dir = try std.fs.openDirAbsolute(exe_dir, .{ .iterate = true });
    defer dir.close();

    var it = dir.iterate();
    while (try it.next()) |entry| {
        if (entry.kind != .file) continue;

        const filename = entry.name;
        if (!std.mem.startsWith(u8, filename, "lib")) continue;
        if (!std.mem.endsWith(u8, filename, lib_ext)) continue;

        std.debug.print("HOST: Found candidate: {s}\n", .{filename});

        const lib_path = try std.fs.path.join(allocator, &[_][]const u8{ exe_dir, filename });
        defer allocator.free(lib_path);

        const lib_path_z = try allocator.dupeZ(u8, lib_path);
        defer allocator.free(lib_path_z);

        const handle = c.dlopen(lib_path_z.ptr, c.RTLD_LAZY);
        if (handle == null) {
            const err = c.dlerror();
            std.debug.print("HOST: Failed to load {s}: {s}\n", .{ filename, err });
            continue;
        }

        const attach_ptr = c.dlsym(handle, "Attach");
        const detach_ptr = c.dlsym(handle, "Detach");
        const invoke_ptr = c.dlsym(handle, "Invoke");

        if (attach_ptr == null or detach_ptr == null or invoke_ptr == null) {
            std.debug.print("HOST: {s} missing required functions (Attach/Detach/Invoke)\n", .{filename});
            _ = c.dlclose(handle);
            continue;
        }

        std.debug.print("HOST: {s} has valid plugin interface\n", .{filename});

        const report_ptr = c.dlsym(handle, "Report");
        if (report_ptr == null) {
            std.debug.print("HOST: {s} missing Report function\n", .{filename});
            _ = c.dlclose(handle);
            continue;
        }

        const report: ReportFn = @ptrCast(@alignCast(report_ptr));
        var info: LibsInfo = undefined;
        var err_buf: [256]u8 = undefined;

        if (!report(&err_buf, err_buf.len, &info)) {
            const err_msg = std.mem.sliceTo(&err_buf, 0);
            std.debug.print("HOST: {s} Report failed: {s}\n", .{ filename, err_msg });
            _ = c.dlclose(handle);
            continue;
        }

        const plugin_type = std.mem.span(info.plugin_type);

        if (std.mem.eql(u8, plugin_type, "control")) {
            std.debug.print("HOST: {s} identified as control plugin (type={s}, id=0x{x})\n", .{ filename, plugin_type, info.plugin_id });

            return ControlFns{
                .handle = handle.?,
                .attach = @ptrCast(@alignCast(attach_ptr)),
                .detach = @ptrCast(@alignCast(detach_ptr)),
                .invoke = @ptrCast(@alignCast(invoke_ptr)),
            };
        }

        std.debug.print("HOST: {s} is not control plugin (type={s})\n", .{ filename, plugin_type });
        _ = c.dlclose(handle);
    }

    std.debug.print("Error: No control plugin found in {s}\n", .{exe_dir});
    return null;
}

fn controlBind() !bool {
    std.debug.print("HOST: Validating control plugin...\n", .{});
    return true;
}

fn controlAttach(fns: ControlFns) !bool {
    std.debug.print("HOST: Attaching to control plugin...\n", .{});

    var err_buf: [256]u8 = undefined;
    const result = fns.attach(fns.invoke, &err_buf, err_buf.len);

    if (!result) {
        const err_msg = std.mem.sliceTo(&err_buf, 0);
        std.debug.print("Error: Control plugin Attach failed: {s}\n", .{err_msg});
        return false;
    }

    std.debug.print("Control plugin attached successfully\n", .{});
    return true;
}

fn controlInvoke(fns: ControlFns) !void {
    std.debug.print("HOST: Invoking control plugin...\n", .{});
    const result = fns.invoke("control.run", "", "");
    _ = result;
}

fn controlDetach(fns: ControlFns) void {
    var err_buf: [256]u8 = undefined;
    _ = fns.detach(&err_buf, err_buf.len);
    _ = c.dlclose(fns.handle);
}

pub fn main() !void {
    std.debug.print("=== ZIG HOST ===\n", .{});

    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();

    const fns = try controlBoot(allocator) orelse {
        std.process.exit(1);
    };

    if (!try controlBind()) {
        std.process.exit(1);
    }

    if (!try controlAttach(fns)) {
        std.process.exit(1);
    }

    try controlInvoke(fns);
    controlDetach(fns);

    std.debug.print("=== ZIG HOST COMPLETE ===\n", .{});
}
