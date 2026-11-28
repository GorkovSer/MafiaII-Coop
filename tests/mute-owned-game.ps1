[CmdletBinding(DefaultParameterSetName='Mute')]
param([Parameter(Mandatory=$true,ParameterSetName='Mute')][int]$GamePid,
      [Parameter(Mandatory=$true,ParameterSetName='Initialize')][switch]$InitializeOnly)
$ErrorActionPreference='Stop'
if(!$InitializeOnly) {
$game=Get-Process -Id $GamePid
if((Get-FileHash -LiteralPath $game.Path).Hash.ToLowerInvariant() -ne '16c79a3be483e582719e43d7b451e887e89414be81ca7ecfc0536dd8185e6c2f'){throw 'Unsupported game process'}
}
if(!('CoopAudioMute' -as [type])) {
Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;
[ComImport,Guid("BCDE0395-E52F-467C-8E3D-C4579291692E")] class MMEnum {}
[ComImport,Guid("A95664D2-9614-4F35-A746-DE8DB63617E6"),InterfaceType(ComInterfaceType.InterfaceIsIUnknown)] interface IMMEnum {
 [PreserveSig] int EnumAudioEndpoints(int flow,uint mask,out IMMCollection devices);
}
[ComImport,Guid("0BD7A1BE-7A1A-44DB-8397-CC5392387B5E"),InterfaceType(ComInterfaceType.InterfaceIsIUnknown)] interface IMMCollection {[PreserveSig] int GetCount(out uint count);[PreserveSig] int Item(uint n,out IMMDevice device);}
[ComImport,Guid("D666063F-1587-4E43-81F1-B948E807363F"),InterfaceType(ComInterfaceType.InterfaceIsIUnknown)] interface IMMDevice {[PreserveSig] int Activate(ref Guid iid,uint cls,IntPtr p,[MarshalAs(UnmanagedType.IUnknown)] out object value);}
[ComImport,Guid("77AA99A0-1BD6-484F-8BC7-2C654C9A9B6F"),InterfaceType(ComInterfaceType.InterfaceIsIUnknown)] interface IManager {
 [PreserveSig] int GetAudioSessionControl(IntPtr p,uint flags,out IntPtr v);[PreserveSig] int GetSimpleAudioVolume(IntPtr p,uint flags,out IntPtr v);[PreserveSig] int GetSessionEnumerator(out IEnumeration e);
}
[ComImport,Guid("E2F5BB11-0570-40CA-ACDD-3AA01277DEE8"),InterfaceType(ComInterfaceType.InterfaceIsIUnknown)] interface IEnumeration {[PreserveSig] int GetCount(out int n);[PreserveSig] int GetSession(int i,[MarshalAs(UnmanagedType.IUnknown)] out object s);}
[ComImport,Guid("BFB7FF88-7239-4FC9-8FA2-07C950BE9C6D"),InterfaceType(ComInterfaceType.InterfaceIsIUnknown)] interface IControl {
 [PreserveSig] int GetState(out int s);[PreserveSig] int GetDisplayName(out IntPtr s);[PreserveSig] int SetDisplayName(IntPtr s,IntPtr g);[PreserveSig] int GetIconPath(out IntPtr s);[PreserveSig] int SetIconPath(IntPtr s,IntPtr g);[PreserveSig] int GetGroupingParam(out Guid g);[PreserveSig] int SetGroupingParam(ref Guid g,IntPtr p);[PreserveSig] int Register(IntPtr p);[PreserveSig] int Unregister(IntPtr p);[PreserveSig] int GetSessionIdentifier(out IntPtr s);[PreserveSig] int GetSessionInstanceIdentifier(out IntPtr s);[PreserveSig] int GetProcessId(out uint p);
}
[ComImport,Guid("87CE5498-68D6-44E5-9215-6DA47EF883D8"),InterfaceType(ComInterfaceType.InterfaceIsIUnknown)] interface IVolume {[PreserveSig] int SetMasterVolume(float v,IntPtr g);[PreserveSig] int GetMasterVolume(out float v);[PreserveSig] int SetMute([MarshalAs(UnmanagedType.Bool)] bool mute,IntPtr g);}
public static class CoopAudioMute {
 public static int Apply(uint pid) {
  int found=0; IMMEnum enumerator=(IMMEnum)new MMEnum(); IMMCollection devices;
  try {Marshal.ThrowExceptionForHR(enumerator.EnumAudioEndpoints(0,1,out devices));
   try {uint count;devices.GetCount(out count);for(uint d=0;d<count;d++){IMMDevice device;devices.Item(d,out device);object manager=null;
    try {Guid iid=new Guid("77AA99A0-1BD6-484F-8BC7-2C654C9A9B6F");device.Activate(ref iid,23,IntPtr.Zero,out manager);IEnumeration sessions;((IManager)manager).GetSessionEnumerator(out sessions);
     try {int n;sessions.GetCount(out n);for(int i=0;i<n;i++){object session;sessions.GetSession(i,out session);try {uint p;((IControl)session).GetProcessId(out p);if(p==pid){Marshal.ThrowExceptionForHR(((IVolume)session).SetMute(true,IntPtr.Zero));found++;}}finally{Marshal.ReleaseComObject(session);}}}finally{Marshal.ReleaseComObject(sessions);}
    }finally {if(manager!=null)Marshal.ReleaseComObject(manager);Marshal.ReleaseComObject(device);}
   }}finally{Marshal.ReleaseComObject(devices);}
  }finally{Marshal.ReleaseComObject(enumerator);}return found;
 }
}
"@
}
if(!$InitializeOnly){[CoopAudioMute]::Apply([uint32]$GamePid)}
