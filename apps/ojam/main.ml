(* OCaml host implementation using C FFI for dynamic library loading *)

(* Use custom C stubs for dynamic library loading *)
external dlopen : string -> int -> nativeint = "caml_dlopen_custom"
external dlsym : nativeint -> string -> nativeint = "caml_dlsym_custom"
external dlclose : nativeint -> unit = "caml_dlclose_custom"

(* C types for plugin contract *)
type invoke_fn = nativeint
type attach_fn = nativeint -> nativeint -> bytes -> int -> bool
type detach_fn = bytes -> int -> bool

(* LibsInfo struct - 5 fields matching C layout *)
type libs_info = {
  plugin_type: string;
  product: string;
  description_long: string;
  description_short: string;
  plugin_id: int64;
}

type report_fn = bytes -> int -> libs_info option

type control_fns = {
  handle: nativeint;
  attach_ptr: nativeint;
  detach_ptr: nativeint;
  invoke_ptr: nativeint;
}

(* RTLD constants *)
let rtld_lazy = 0x001

(* Foreign function interface stubs *)
external call_attach : nativeint -> nativeint -> bytes -> int -> bool = "caml_call_attach"
external call_detach : nativeint -> bytes -> int -> bool = "caml_call_detach"
external call_invoke : nativeint -> string -> string -> string -> string = "caml_call_invoke"
external call_report : nativeint -> bytes -> int -> libs_info option = "caml_call_report"

(* Step 1: Control Boot - Discover control plugin *)
let control_boot () =
  print_endline "HOST: Discovering control plugin...";
  
  let exe_path = Sys.executable_name in
  let exe_dir = Filename.dirname exe_path in
  Printf.printf "HOST: Scanning directory: %s\n" exe_dir;
  
  let lib_ext = ".dylib" in  (* macOS *)
  
  let files = Sys.readdir exe_dir in
  let candidates = Array.to_list files
    |> List.filter (fun f -> 
        String.length f > 3 && 
        String.sub f 0 3 = "lib" &&
        Filename.check_suffix f lib_ext)
  in
  
  let rec find_control libs = match libs with
    | [] -> 
        print_endline "Error: No control plugin found";
        None
    | filename :: rest ->
        Printf.printf "HOST: Found candidate: %s\n" filename;
        
        let lib_path = Filename.concat exe_dir filename in
        let handle = dlopen lib_path rtld_lazy in
        
        if handle = 0n then begin
          Printf.printf "HOST: Failed to load %s\n" filename;
          find_control rest
        end else begin
          let attach_ptr = dlsym handle "Attach" in
          let detach_ptr = dlsym handle "Detach" in
          let invoke_ptr = dlsym handle "Invoke" in
          
          if attach_ptr = 0n || detach_ptr = 0n || invoke_ptr = 0n then begin
            Printf.printf "HOST: %s missing required functions\n" filename;
            dlclose handle;
            find_control rest
          end else begin
            Printf.printf "HOST: %s has valid plugin interface\n" filename;
            
            let report_ptr = dlsym handle "Report" in
            if report_ptr = 0n then begin
              Printf.printf "HOST: %s missing Report function\n" filename;
              dlclose handle;
              find_control rest
            end else begin
              let err_buf = Bytes.create 256 in
              match call_report report_ptr err_buf 256 with
              | None ->
                  let err_msg = Bytes.to_string err_buf in
                  Printf.printf "HOST: %s Report failed: %s\n" filename err_msg;
                  dlclose handle;
                  find_control rest
              | Some info ->
                  if info.plugin_type = "control" then begin
                    Printf.printf "HOST: %s identified as control plugin (type=%s, id=0x%Lx)\n" 
                      filename info.plugin_type info.plugin_id;
                    Some {
                      handle = handle;
                      attach_ptr = attach_ptr;
                      detach_ptr = detach_ptr;
                      invoke_ptr = invoke_ptr;
                    }
                  end else begin
                    Printf.printf "HOST: %s is not control plugin (type=%s)\n" 
                      filename info.plugin_type;
                    dlclose handle;
                    find_control rest
                  end
            end
          end
        end
  in
  find_control candidates

(* Step 2: Control Bind - Validate plugin functions (already done in boot) *)
let control_bind fns =
  if fns.attach_ptr = 0n || fns.detach_ptr = 0n || fns.invoke_ptr = 0n then begin
    print_endline "Error: Control plugin missing required functions";
    false
  end else begin
    print_endline "Resolved control plugin functions (Attach/Detach/Invoke)";
    true
  end

(* Step 3: Control Attach - Pass control's own Invoke as dispatch *)
let control_attach fns =
  let err_buf = Bytes.create 256 in
  let result = call_attach fns.attach_ptr fns.invoke_ptr err_buf 256 in
  
  if not result then begin
    let err_msg = Bytes.to_string err_buf in
    Printf.printf "Error: Control plugin Attach failed: %s\n" err_msg;
    false
  end else begin
    print_endline "Control plugin attached successfully";
    true
  end

(* Step 4: Control Invoke - Execute plugin functionality *)
let control_invoke fns =
  let result = call_invoke fns.invoke_ptr "control.run" "{}" "{}" in
  Printf.printf "Control plugin result: %s\n" result

(* Step 5: Control Detach - Cleanup *)
let control_detach fns =
  let err_buf = Bytes.create 256 in
  let _ = call_detach fns.detach_ptr err_buf 256 in
  dlclose fns.handle

let () =
  print_endline "=== OCAML HOST ===";
  
  match control_boot () with
  | None -> exit 1
  | Some fns ->
      if not (control_bind fns) then exit 1
      else if not (control_attach fns) then exit 1
      else begin
        control_invoke fns;
        control_detach fns;
        print_endline "=== OCAML HOST COMPLETE ==="
      end