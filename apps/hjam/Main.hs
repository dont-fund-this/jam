{-# LANGUAGE ForeignFunctionInterface #-}

module Main where

import Foreign
import Foreign.C.String
import Foreign.C.Types
import System.Posix.DynamicLinker
import System.Environment (getExecutablePath)
import System.FilePath (takeDirectory, (</>))
import System.Directory (listDirectory)
import Control.Monad (forM_, filterM)
import Data.List (isPrefixOf, isSuffixOf)
import Data.Maybe (isJust, fromJust)

-- C types matching plugin contract
type InvokeFn = CString -> CString -> CString -> IO CString
type AttachFn = FunPtr InvokeFn -> CString -> CSize -> IO CBool
type DetachFn = CString -> CSize -> IO CBool

-- LibsInfo struct matching C layout
data LibsInfo = LibsInfo
  { pluginType :: CString
  , product :: CString
  , descriptionLong :: CString
  , descriptionShort :: CString
  , pluginId :: Word64
  }

instance Storable LibsInfo where
  sizeOf _ = 40  -- 5 pointers * 8 bytes
  alignment _ = 8
  peek ptr = do
    pType <- peekByteOff ptr 0
    pProd <- peekByteOff ptr 8
    pDescLong <- peekByteOff ptr 16
    pDescShort <- peekByteOff ptr 24
    pId <- peekByteOff ptr 32
    return $ LibsInfo pType pProd pDescLong pDescShort pId
  poke ptr (LibsInfo pType pProd pDescLong pDescShort pId) = do
    pokeByteOff ptr 0 pType
    pokeByteOff ptr 8 pProd
    pokeByteOff ptr 16 pDescLong
    pokeByteOff ptr 24 pDescShort
    pokeByteOff ptr 32 pId

type ReportFn = CString -> CSize -> Ptr LibsInfo -> IO CBool

data ControlFns = ControlFns
  { ctrlHandle :: DL
  , ctrlAttach :: FunPtr AttachFn
  , ctrlDetach :: FunPtr DetachFn
  , ctrlInvoke :: FunPtr InvokeFn
  }

-- Foreign imports for dynamic function pointers
foreign import ccall "dynamic"
  mkAttachFn :: FunPtr AttachFn -> AttachFn

foreign import ccall "dynamic"
  mkDetachFn :: FunPtr DetachFn -> DetachFn

foreign import ccall "dynamic"
  mkInvokeFn :: FunPtr InvokeFn -> InvokeFn

foreign import ccall "dynamic"
  mkReportFn :: FunPtr ReportFn -> ReportFn

foreign import ccall "wrapper"
  wrapInvokeFn :: InvokeFn -> IO (FunPtr InvokeFn)

-- Main dispatch callback
dispatch :: InvokeFn
dispatch addr args meta = do
  addrStr <- peekCString addr
  return nullPtr

-- Step 1: Control Boot - Discover control plugin
controlBoot :: IO (Maybe ControlFns)
controlBoot = do
  putStrLn "HOST: Discovering control plugin..."
  
  exePath <- getExecutablePath
  let exeDir = takeDirectory exePath
  putStrLn $ "HOST: Scanning directory: " ++ exeDir
  
  let libExt = ".dylib"  -- macOS
  
  files <- listDirectory exeDir
  let candidates = filter (\f -> "lib" `isPrefixOf` f && libExt `isSuffixOf` f) files
  
  result <- findControlPlugin exeDir candidates
  return result

findControlPlugin :: FilePath -> [FilePath] -> IO (Maybe ControlFns)
findControlPlugin _ [] = do
  putStrLn "Error: No control plugin found"
  return Nothing
findControlPlugin exeDir (filename:rest) = do
  putStrLn $ "HOST: Found candidate: " ++ filename
  
  let libPath = exeDir </> filename
  dlHandle <- dlopen libPath [RTLD_LAZY]
  
  -- Check for required functions
  attachPtr <- dlsym dlHandle "Attach"
  detachPtr <- dlsym dlHandle "Detach"
  invokePtr <- dlsym dlHandle "Invoke"
  
  if nullFunPtr == attachPtr || nullFunPtr == detachPtr || nullFunPtr == invokePtr
    then do
      putStrLn $ "HOST: " ++ filename ++ " missing required functions"
      dlclose dlHandle
      findControlPlugin exeDir rest
    else do
      putStrLn $ "HOST: " ++ filename ++ " has valid plugin interface"
      
      -- Check for Report function
      reportPtr <- dlsym dlHandle "Report"
      if nullFunPtr == reportPtr
        then do
          putStrLn $ "HOST: " ++ filename ++ " missing Report function"
          dlclose dlHandle
          findControlPlugin exeDir rest
        else do
          let reportFn = mkReportFn reportPtr
          
          -- Call Report to get plugin info
          allocaBytes 256 $ \errBuf -> 
            alloca $ \infoPtr -> do
              result <- reportFn errBuf 256 infoPtr
              
              if result == 0
                then do
                  errMsg <- peekCString errBuf
                  putStrLn $ "HOST: " ++ filename ++ " Report failed: " ++ errMsg
                  dlclose dlHandle
                  findControlPlugin exeDir rest
                else do
                  info <- peek infoPtr
                  pType <- peekCString (pluginType info)
                  
                  if pType == "control"
                    then do
                      let pId = pluginId info
                      putStrLn $ "HOST: " ++ filename ++ " identified as control plugin (type=" ++ pType ++ ", id=0x" ++ show pId ++ ")"
                      
                      return $ Just ControlFns
                        { ctrlHandle = dlHandle
                        , ctrlAttach = attachPtr
                        , ctrlDetach = detachPtr
                        , ctrlInvoke = invokePtr
                        }
                    else do
                      putStrLn $ "HOST: " ++ filename ++ " is not control plugin (type=" ++ pType ++ ")"
                      dlclose dlHandle
                      findControlPlugin exeDir rest

-- Step 2: Control Bind - Validate plugin functions
controlBind :: IO Bool
controlBind = do
  putStrLn "HOST: Validating control plugin..."
  return True

-- Step 3: Control Attach - Register dispatch callback
controlAttach :: ControlFns -> IO Bool
controlAttach fns = do
  putStrLn "HOST: Attaching to control plugin..."
  
  dispatchPtr <- wrapInvokeFn dispatch
  let attachFn = mkAttachFn (ctrlAttach fns)
  
  allocaBytes 256 $ \errBuf -> do
    result <- attachFn dispatchPtr errBuf 256
    
    if result == 0
      then do
        errMsg <- peekCString errBuf
        putStrLn $ "Error: Control plugin Attach failed: " ++ errMsg
        return False
      else do
        putStrLn "Control plugin attached successfully"
        return True

-- Step 4: Control Invoke - Execute plugin functionality
controlInvoke :: ControlFns -> IO ()
controlInvoke fns = do
  putStrLn "HOST: Invoking control plugin..."
  
  let invokeFn = mkInvokeFn (ctrlInvoke fns)
  
  withCString "control.run" $ \addr ->
    withCString "" $ \args ->
      withCString "" $ \meta -> do
        _ <- invokeFn addr args meta
        return ()

-- Step 5: Control Detach - Cleanup
controlDetach :: ControlFns -> IO ()
controlDetach fns = do
  let detachFn = mkDetachFn (ctrlDetach fns)
  
  allocaBytes 256 $ \errBuf -> do
    _ <- detachFn errBuf 256
    dlclose (ctrlHandle fns)

main :: IO ()
main = do
  putStrLn "=== HASKELL HOST ==="
  
  maybeFns <- controlBoot
  
  case maybeFns of
    Nothing -> return ()
    Just fns -> do
      bindOk <- controlBind
      if not bindOk
        then return ()
        else do
          attachOk <- controlAttach fns
          if not attachOk
            then return ()
            else do
              controlInvoke fns
              controlDetach fns
              putStrLn "=== HASKELL HOST COMPLETE ==="
