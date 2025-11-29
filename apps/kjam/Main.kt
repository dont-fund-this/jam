import com.sun.jna.Library
import com.sun.jna.Native
import com.sun.jna.Callback
import com.sun.jna.Structure
import com.sun.jna.Pointer
import java.io.File

interface PluginLibrary : Library {
    fun interface InvokeFn : Callback {
        fun invoke(address: String?, payload: String?, options: String?): String?
    }
    
    fun Invoke(address: String?, payload: String?, options: String?): String?
    fun Attach(dispatch: InvokeFn, errBuf: ByteArray, errCap: Int): Boolean
    fun Detach(errBuf: ByteArray, errCap: Int): Boolean
    fun Report(errBuf: ByteArray, errCap: Int, out: LibsInfo.ByReference): Boolean
}

@Structure.FieldOrder("plugin_type", "product", "description_long", "description_short", "plugin_id")
open class LibsInfo : Structure() {
    @JvmField var plugin_type: String? = null
    @JvmField var product: String? = null
    @JvmField var description_long: String? = null
    @JvmField var description_short: String? = null
    @JvmField var plugin_id: Long = 0
    
    class ByReference : LibsInfo(), Structure.ByReference
}

data class ControlFns(
    val lib: PluginLibrary,
    val invokeFn: PluginLibrary.InvokeFn? = null
)

fun controlBoot(): ControlFns? {
    println("HOST: Discovering control plugin...")
    
    val jarFile = File(object {}.javaClass.protectionDomain.codeSource.location.toURI())
    val exeDir = jarFile.parentFile
    
    println("HOST: Scanning directory: ${exeDir.absolutePath}")
    
    val libExt = when {
        System.getProperty("os.name").lowercase().contains("win") -> ".dll"
        System.getProperty("os.name").lowercase().contains("mac") -> ".dylib"
        else -> ".so"
    }
    
    val files = exeDir.listFiles() ?: run {
        System.err.println("Error: Cannot read directory $exeDir")
        return null
    }
    
    for (file in files) {
        if (!file.isFile) continue
        
        val filename = file.name
        
        if (!filename.endsWith(libExt)) continue
        if (!filename.startsWith("lib")) continue
        
        println("HOST: Found candidate: $filename")
        
        try {
            val lib = Native.load(file.absolutePath, PluginLibrary::class.java)
            
            println("HOST: $filename has valid plugin interface")
            
            val errBuf = ByteArray(256)
            val info = LibsInfo.ByReference()
            
            if (!lib.Report(errBuf, errBuf.size, info)) {
                val errMsg = String(errBuf).trim('\u0000')
                println("HOST: $filename Report failed: $errMsg")
                continue
            }
            
            val pluginType = info.plugin_type
            
            if (pluginType == "control") {
                println("HOST: $filename identified as control plugin (type=$pluginType, id=0x${info.plugin_id.toString(16)})")
                return ControlFns(lib)
            }
            
            println("HOST: $filename is not control plugin (type=$pluginType)")
            
        } catch (e: UnsatisfiedLinkError) {
            println("HOST: Failed to load $filename: ${e.message}")
        }
    }
    
    System.err.println("Error: No control plugin found in $exeDir")
    return null
}

fun controlBind(fns: ControlFns): Boolean {
    println("Resolved control plugin functions (Attach/Detach/Invoke)")
    return true
}

fun controlAttach(fns: ControlFns): Boolean {
    println("Resolved control plugin functions (Attach/Detach/Invoke)")
    
    val invokeCallback = PluginLibrary.InvokeFn { address, payload, options ->
        fns.lib.Invoke(address, payload, options)
    }
    
    val errBuf = ByteArray(256)
    val result = fns.lib.Attach(invokeCallback, errBuf, errBuf.size)
    
    if (!result) {
        val errMsg = String(errBuf).trim('\u0000')
        System.err.println("Error: Control plugin Attach failed: $errMsg")
        return false
    }
    
    println("Control plugin attached successfully")
    return true
}

fun controlInvoke(fns: ControlFns) {
    val response = fns.lib.Invoke("control.run", "{}", "{}")
    println("Control plugin result: $response")
}

fun controlDetach(fns: ControlFns) {
    val errBuf = ByteArray(256)
    fns.lib.Detach(errBuf, errBuf.size)
}

fun main() {
    println("=== KOTLIN HOST ===")
    
    val fns = controlBoot() ?: kotlin.system.exitProcess(1)
    
    if (!controlBind(fns)) {
        kotlin.system.exitProcess(1)
    }
    
    if (!controlAttach(fns)) {
        kotlin.system.exitProcess(1)
    }
    
    controlInvoke(fns)
    controlDetach(fns)
    
    println("=== KOTLIN HOST COMPLETE ===")
}
