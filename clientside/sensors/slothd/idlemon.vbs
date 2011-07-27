Const HIDDEN_WINDOW = 12

strComputer = "."
Set objWMIService = GetObject("winmgmts:" _
    & "{impersonationLevel=impersonate}!\\" & strComputer & "\root\cimv2")
Set objStartup = objWMIService.Get("Win32_ProcessStartup")

Set objConfig = objStartup.SpawnInstance_
objConfig.ShowWindow = HIDDEN_WINDOW
Set objProcess = GetObject("winmgmts:root\cimv2:Win32_Process")
pgm = "C:\cygwin\usr\local\etc\emulab\idlemon.exe"
errReturn = objProcess.Create(pgm, null, objConfig, intProcessID)
