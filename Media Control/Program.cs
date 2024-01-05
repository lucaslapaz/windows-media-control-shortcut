using System;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Windows.Forms;

class Program
{
    private const int WH_KEYBOARD_LL = 13;
    private const int VK_ESCAPE = 0x1B;
    private const int VK_F1 = 0x70;
    private const int VK_F2 = 0x71;
    private const int VK_F3 = 0x72;
    private const int VK_F4 = 0x73;
    private const int VK_F5 = 0x74;
    private const int VK_F6 = 0x75;
    private const int VK_F7 = 0x76;
    private const int VK_F8 = 0x77;
    private const int VK_F9 = 0x78;
    private const int VK_F10 = 0x79;
    private const int VK_F11 = 0x7A;
    private const int VK_F12 = 0x7B;
    private const int VK_CONTROL = 0x11;
    private const int VK_LCONTROL = 0xA2;
    private const int VK_RCONTROL = 0xA3;
    private const int WM_KEYDOWN = 0x0100;
    private const int WM_KEYUP = 0x0101;

    private const int WM_APPCOMMAND = 0x0319;
    private const int APPCOMMAND_MEDIA_PLAY_PAUSE = 0xe0000;
    private const int APPCOMMAND_MEDIA_STOP = 0xd0000;
    private const int APPCOMMAND_MEDIA_PREV_TRACK = 0xc0000;
    private const int APPCOMMAND_MEDIA_NEXT_TRACK = 0xb0000;
    private const int APPCOMMAND_VOLUME_UP = 0xa0000;
    private const int APPCOMMAND_VOLUME_DOWN = 0x90000;
    private const int APPCOMMAND_VOLUME_MUTE = 0x80000;

    private static LowLevelKeyboardProc _proc;
    private static IntPtr _hookID = IntPtr.Zero;

    static void Main()
    {
        Console.WriteLine("Aplicativo iniciado. Ouvindo os atalhos de controle....");
        _proc = HookCallback;
        _hookID = SetHook(_proc);

        Application.Run();  // Mantém o programa em execução para continuar recebendo eventos

        UnhookWindowsHookEx(_hookID);
    }

    private static IntPtr SetHook(LowLevelKeyboardProc proc)
    {
        using (Process curProcess = Process.GetCurrentProcess())
        using (ProcessModule curModule = curProcess.MainModule)
        {
            return SetWindowsHookEx(WH_KEYBOARD_LL, proc, GetModuleHandle(curModule.ModuleName), 0);
        }
    }

    private delegate IntPtr LowLevelKeyboardProc(int nCode, IntPtr wParam, IntPtr lParam);

    private static IntPtr HookCallback(int nCode, IntPtr wParam, IntPtr lParam)
    {
        if (nCode >= 0 && wParam == (IntPtr)WM_KEYDOWN)
        {
            int vkCode = Marshal.ReadInt32(lParam);
            bool ctrlPressed = (GetKeyState(VK_RCONTROL) & 0x8000) != 0;
            int COMMAND;

            if (ctrlPressed)
            {
                if (vkCode == VK_F5)
                {
                    COMMAND = APPCOMMAND_MEDIA_PLAY_PAUSE;
                    Console.WriteLine("Tecla Ctrl + F5 pressionada. Pausando a mídia...");
                }
                else if (vkCode == VK_F6)
                {
                    COMMAND = APPCOMMAND_MEDIA_PREV_TRACK;
                    Console.WriteLine("Tecla Ctrl + F6 pressionada. Voltando a mídia...");
                }
                else if (vkCode == VK_F7)
                {
                    COMMAND = APPCOMMAND_MEDIA_NEXT_TRACK;
                    Console.WriteLine("Tecla Ctrl + F7 pressionada. Pulando a mídia...");
                }
                else if (vkCode == VK_F8)
                {
                    COMMAND = APPCOMMAND_VOLUME_DOWN;
                    Console.WriteLine("Tecla Ctrl + F8 pressionada. Diminuindo o volume...");
                }
                else if (vkCode == VK_F9)
                {
                    COMMAND = APPCOMMAND_VOLUME_UP;
                    Console.WriteLine("Tecla Ctrl + F9 pressionada. Aumentando o volume...");
                }
                else if (vkCode == VK_F10)
                {
                    COMMAND = APPCOMMAND_VOLUME_MUTE;
                    Console.WriteLine("Tecla Ctrl + F10 pressionada. Áudio mutado...");
                }
                else
                {
                    return CallNextHookEx(_hookID, nCode, wParam, lParam);
                }
                SendMediaCommand(COMMAND);
            }
        }
        return CallNextHookEx(_hookID, nCode, wParam, lParam);
    }

    private static void SendMediaCommand(int appCommand)
    {
        Process currentProcess = Process.GetCurrentProcess();
        IntPtr mainWindowHandle = currentProcess.MainWindowHandle;

        SendMessageW(mainWindowHandle, WM_APPCOMMAND, IntPtr.Zero, (IntPtr)appCommand);
    }

    #region Importação de Funções da API do Windows
    [DllImport("user32.dll", CharSet = CharSet.Auto, SetLastError = true)]
    private static extern IntPtr SetWindowsHookEx(int idHook, LowLevelKeyboardProc lpfn, IntPtr hMod, uint dwThreadId);

    [DllImport("user32.dll", CharSet = CharSet.Auto, SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool UnhookWindowsHookEx(IntPtr hhk);

    [DllImport("user32.dll", CharSet = CharSet.Auto, SetLastError = true)]
    private static extern IntPtr CallNextHookEx(IntPtr hhk, int nCode, IntPtr wParam, IntPtr lParam);

    [DllImport("kernel32.dll", CharSet = CharSet.Auto, SetLastError = true)]
    private static extern IntPtr GetModuleHandle(string lpModuleName);

    [DllImport("user32.dll")]
    private static extern IntPtr SendMessageW(IntPtr hWnd, int Msg, IntPtr wParam, IntPtr lParam);

    [DllImport("user32.dll", CharSet = CharSet.Auto, SetLastError = true)]
    private static extern short GetKeyState(int vKey);
    #endregion
}
