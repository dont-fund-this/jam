import com.sun.jna.Library;
import com.sun.jna.Native;
import com.sun.jna.Pointer;
import com.sun.jna.Structure;
import com.sun.jna.Callback;
import java.io.File;
import java.util.Arrays;
import java.util.List;

public class Main {
    
    public interface PluginLibrary extends Library {
        interface InvokeFn extends Callback {
            String invoke(String address, String payload, String options);
        }
        
        interface AttachFn extends Callback {
            boolean call(InvokeFn dispatch, byte[] err_buf, int err_cap);
        }
        
        interface DetachFn extends Callback {
            boolean call(byte[] err_buf, int err_cap);
        }
        
        interface ReportFn extends Callback {
            boolean call(byte[] err_buf, int err_cap, LibsInfo.ByReference out);
        }
        
        String Invoke(String address, String payload, String options);
        boolean Attach(InvokeFn dispatch, byte[] err_buf, int err_cap);
        boolean Detach(byte[] err_buf, int err_cap);
        boolean Report(byte[] err_buf, int err_cap, LibsInfo.ByReference out);
    }
    
    public static class LibsInfo extends Structure {
        public String plugin_type;
        public String product;
        public String description_long;
        public String description_short;
        public long plugin_id;
        
        public static class ByReference extends LibsInfo implements Structure.ByReference {}
        
        @Override
        protected List<String> getFieldOrder() {
            return Arrays.asList("plugin_type", "product", "description_long", "description_short", "plugin_id");
        }
    }
    
    static class ControlFns {
        PluginLibrary lib;
        PluginLibrary.InvokeFn invokeFn;
        PluginLibrary.AttachFn attachFn;
        PluginLibrary.DetachFn detachFn;
    }
    
    static ControlFns controlBoot() {
        System.out.println("HOST: Discovering control plugin...");
        
        // Get directory where jjam.jar is located
        File jarFile = new File(Main.class.getProtectionDomain().getCodeSource().getLocation().getPath());
        File exeDir = jarFile.getParentFile();
        
        System.out.println("HOST: Scanning directory: " + exeDir.getAbsolutePath());
        
        String libExt = ".dylib";
        String os = System.getProperty("os.name").toLowerCase();
        if (os.contains("win")) {
            libExt = ".dll";
        } else if (os.contains("nix") || os.contains("nux")) {
            libExt = ".so";
        }
        
        File[] files = exeDir.listFiles();
        if (files == null) {
            System.err.println("Error: Cannot read directory " + exeDir);
            return null;
        }
        
        for (File file : files) {
            if (!file.isFile()) continue;
            
            String filename = file.getName();
            
            if (!filename.endsWith(libExt)) continue;
            if (!filename.startsWith("lib")) continue;
            
            System.out.println("HOST: Found candidate: " + filename);
            
            try {
                PluginLibrary lib = Native.load(file.getAbsolutePath(), PluginLibrary.class);
                
                System.out.println("HOST: " + filename + " has valid plugin interface");
                
                // Call Report to check plugin type
                byte[] errBuf = new byte[256];
                LibsInfo.ByReference info = new LibsInfo.ByReference();
                
                if (!lib.Report(errBuf, errBuf.length, info)) {
                    System.out.println("HOST: " + filename + " Report failed: " + new String(errBuf).trim());
                    continue;
                }
                
                String pluginType = info.plugin_type;
                
                if ("control".equals(pluginType)) {
                    System.out.printf("HOST: %s identified as control plugin (type=%s, id=0x%x)%n",
                        filename, pluginType, info.plugin_id);
                    
                    ControlFns fns = new ControlFns();
                    fns.lib = lib;
                    return fns;
                }
                
                System.out.println("HOST: " + filename + " is not control plugin (type=" + pluginType + ")");
                
            } catch (UnsatisfiedLinkError e) {
                System.out.println("HOST: Failed to load " + filename + ": " + e.getMessage());
            }
        }
        
        System.err.println("Error: No control plugin found in " + exeDir);
        return null;
    }
    
    static boolean controlBind(ControlFns fns) {
        if (fns.lib == null) {
            System.err.println("Error: Control plugin missing required functions");
            return false;
        }
        System.out.println("Resolved control plugin functions (Attach/Detach/Invoke)");
        return true;
    }
    
    static boolean controlAttach(ControlFns fns) {
        System.out.println("Resolved control plugin functions (Attach/Detach/Invoke)");
        
        // Create callback wrapper
        PluginLibrary.InvokeFn invokeCallback = new PluginLibrary.InvokeFn() {
            public String invoke(String address, String payload, String options) {
                return fns.lib.Invoke(address, payload, options);
            }
        };
        
        // Call Attach
        byte[] errBuf = new byte[256];
        boolean result = fns.lib.Attach(invokeCallback, errBuf, errBuf.length);
        
        if (!result) {
            System.err.println("Error: Control plugin Attach failed: " + new String(errBuf).trim());
            return false;
        }
        
        System.out.println("Control plugin attached successfully");
        return true;
    }
    
    static void controlInvoke(ControlFns fns) {
        String response = fns.lib.Invoke("control.run", "{}", "{}");
        System.out.println("Control plugin result: " + response);
    }
    
    static void controlDetach(ControlFns fns) {
        byte[] errBuf = new byte[256];
        fns.lib.Detach(errBuf, errBuf.length);
    }
    
    public static void main(String[] args) {
        System.out.println("=== JAVA HOST ===");
        
        ControlFns fns = controlBoot();
        if (fns == null) {
            System.exit(1);
        }
        
        if (!controlBind(fns)) {
            System.exit(1);
        }
        
        if (!controlAttach(fns)) {
            System.exit(1);
        }
        
        controlInvoke(fns);
        controlDetach(fns);
        
        System.out.println("=== JAVA HOST COMPLETE ===");
    }
}
