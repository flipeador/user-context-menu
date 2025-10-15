using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Navigation;
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using UserContextMenuApp.View;
using Windows.Win32;
using Windows.Win32.Foundation;
using Windows.Win32.UI.WindowsAndMessaging;

namespace UserContextMenuApp
{
    public delegate nint? WindowMessageHandler(uint msg, nint wParam, nint lParam);

    public sealed partial class MainWindow : Window
    {
        private readonly WNDPROC m_windowProc;
        private readonly WNDPROC m_oldWindowProc;

        private readonly List<WindowMessageHandler> m_messageHandlers = [];

        public MainWindow()
        {
            InitializeComponent();

            Title = "User Context Menu | Flipeador";
            Util.SetWindowTitleBar(this, e_titleBar);

            m_windowProc = WindowProc;
            m_oldWindowProc = Marshal.GetDelegateForFunctionPointer<WNDPROC>(
                PInvoke.SetWindowLongPtr(
                    new HWND(Util.GetWindowHandle(this)),
                    WINDOW_LONG_PTR_INDEX.GWLP_WNDPROC,
                    Marshal.GetFunctionPointerForDelegate(m_windowProc)
                )
            );
        }

        private void Initialize(object sender, RoutedEventArgs args)
        {
            e_rootFrame.NavigationFailed += OnNavigationFailed;
            e_rootFrame.Navigate(typeof(MainPage), this);
        }

        private void OnNavigationFailed(object sender, NavigationFailedEventArgs args)
        {
            throw new Exception($"Failed to load Page: {args.SourcePageType.FullName}");
        }

        private void ToggleTheme(object sender, RoutedEventArgs e)
        {
            Util.ToggleTheme(this);
        }

        public void AddMessageHandler(WindowMessageHandler handler)
        {
            m_messageHandlers.Add(handler);
        }

        public void RemoveMessageHandler(WindowMessageHandler handler)
        {
            m_messageHandlers.Remove(handler);
        }

        private LRESULT WindowProc(HWND hWnd, uint msg, WPARAM wParam, LPARAM lParam)
        {
            foreach (var handler in m_messageHandlers)
            {
                var result = handler(msg, (nint)wParam.Value, lParam);
                if (result != null) return new LRESULT((nint)result);
            }

            return PInvoke.CallWindowProc(m_oldWindowProc, hWnd, msg, wParam, lParam);
        }
    }
}
