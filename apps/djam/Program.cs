using System;
using System.IO;
using System.Runtime.InteropServices;

class Program
{
    // C function pointer types matching plugin contract
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    delegate IntPtr InvokeFn(
        [MarshalAs(UnmanagedType.LPStr)] string address,
        [MarshalAs(UnmanagedType.LPStr)] string payload,
        [MarshalAs(UnmanagedType.LPStr)] string options
    );

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    delegate int AttachFn(InvokeFn dispatch, byte[] errBuf, int errCap);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    delegate int DetachFn(byte[] errBuf, int errCap);

    [StructLayout(LayoutKind.Sequential)]
    struct LibsInfo
    {
        public IntPtr plugin_type;
        public IntPtr product;
        public IntPtr description_long;
        public IntPtr description_short;
        public ulong plugin_id;
    }

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    delegate int ReportFn(byte[] errBuf, int errCap, ref LibsInfo info);

    class ControlFns
    {
        public IntPtr handle;
        public AttachFn? attach;
        public DetachFn? detach;
        public InvokeFn? invoke;
    }

    static ControlFns? ControlBoot()
    {
        Console.WriteLine("HOST: Discovering control plugin...");

        string exePath = System.Reflection.Assembly.GetExecutingAssembly().Location;
        string exeDir = Path.GetDirectoryName(exePath) ?? ".";

        Console.WriteLine($"HOST: Scanning directory: {exeDir}");

        string libExt;
        if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
            libExt = ".dll";
        else if (RuntimeInformation.IsOSPlatform(OSPlatform.OSX))
            libExt = ".dylib";
        else
            libExt = ".so";

        string[] files;
        try
        {
            files = Directory.GetFiles(exeDir);
        }
        catch (Exception e)
        {
            Console.Error.WriteLine($"Error: Failed to read directory: {e.Message}");
            return null;
        }

        foreach (string filePath in files)
        {
            string filename = Path.GetFileName(filePath);

            if (!filename.EndsWith(libExt))
                continue;

            if (!filename.StartsWith("lib"))
                continue;

            Console.WriteLine($"HOST: Found candidate: {filename}");

            IntPtr handle;
            try
            {
                handle = NativeLibrary.Load(filePath);
            }
            catch (Exception e)
            {
                Console.WriteLine($"HOST: Failed to load {filename}: {e.Message}");
                continue;
            }

            // Check for required functions
            if (!NativeLibrary.TryGetExport(handle, "Attach", out IntPtr attachPtr) ||
                !NativeLibrary.TryGetExport(handle, "Detach", out IntPtr detachPtr) ||
                !NativeLibrary.TryGetExport(handle, "Invoke", out IntPtr invokePtr))
            {
                Console.WriteLine($"HOST: {filename} missing required functions (Attach/Detach/Invoke)");
                NativeLibrary.Free(handle);
                continue;
            }

            Console.WriteLine($"HOST: {filename} has valid plugin interface");

            if (!NativeLibrary.TryGetExport(handle, "Report", out IntPtr reportPtr))
            {
                Console.WriteLine($"HOST: {filename} missing Report function");
                NativeLibrary.Free(handle);
                continue;
            }

            // Call Report to check plugin type
            var report = Marshal.GetDelegateForFunctionPointer<ReportFn>(reportPtr);
            var info = new LibsInfo();
            byte[] errBuf = new byte[256];

            if (report(errBuf, errBuf.Length, ref info) == 0)
            {
                string errMsg = System.Text.Encoding.UTF8.GetString(errBuf).TrimEnd('\0');
                Console.WriteLine($"HOST: {filename} Report failed: {errMsg}");
                NativeLibrary.Free(handle);
                continue;
            }

            string pluginType = Marshal.PtrToStringAnsi(info.plugin_type) ?? "";

            if (pluginType == "control")
            {
                Console.WriteLine($"HOST: {filename} identified as control plugin (type={pluginType}, id=0x{info.plugin_id:x})");
                
                return new ControlFns
                {
                    handle = handle,
                    attach = Marshal.GetDelegateForFunctionPointer<AttachFn>(attachPtr),
                    detach = Marshal.GetDelegateForFunctionPointer<DetachFn>(detachPtr),
                    invoke = Marshal.GetDelegateForFunctionPointer<InvokeFn>(invokePtr)
                };
            }

            Console.WriteLine($"HOST: {filename} is not control plugin (type={pluginType})");
            NativeLibrary.Free(handle);
        }

        Console.Error.WriteLine($"Error: No control plugin found in {exeDir}");
        return null;
    }

    static bool ControlBind(ControlFns fns)
    {
        if (fns.attach == null || fns.detach == null || fns.invoke == null)
        {
            Console.Error.WriteLine("Error: Control plugin missing required functions");
            return false;
        }
        Console.WriteLine("Resolved control plugin functions (Attach/Detach/Invoke)");
        return true;
    }

    static bool ControlAttach(ControlFns fns)
    {
        Console.WriteLine("Resolved control plugin functions (Attach/Detach/Invoke)");

        byte[] errBuf = new byte[256];
        int result = fns.attach!(fns.invoke!, errBuf, errBuf.Length);

        if (result == 0)
        {
            string errMsg = System.Text.Encoding.UTF8.GetString(errBuf).TrimEnd('\0');
            Console.Error.WriteLine($"Error: Control plugin Attach failed: {errMsg}");
            return false;
        }

        Console.WriteLine("Control plugin attached successfully");
        return true;
    }

    static void ControlInvoke(ControlFns fns)
    {
        IntPtr response = fns.invoke!("control.run", "{}", "{}");
        string responseStr = Marshal.PtrToStringAnsi(response) ?? "";
        Console.WriteLine($"Control plugin result: {responseStr}");
    }

    static void ControlDetach(ControlFns fns)
    {
        byte[] errBuf = new byte[256];
        fns.detach!(errBuf, errBuf.Length);
        if (fns.handle != IntPtr.Zero)
        {
            NativeLibrary.Free(fns.handle);
        }
    }

    static int Main(string[] args)
    {
        Console.WriteLine("=== C# HOST ===");

        var fns = ControlBoot();
        if (fns == null)
            return 1;

        if (!ControlBind(fns))
            return 1;

        if (!ControlAttach(fns))
            return 1;

        ControlInvoke(fns);
        ControlDetach(fns);

        Console.WriteLine("=== C# HOST COMPLETE ===");
        return 0;
    }
}
