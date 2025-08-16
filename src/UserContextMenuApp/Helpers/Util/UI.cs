using System;
using CommunityToolkit.WinUI.Helpers;
using Microsoft.UI.Windowing;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Windows.UI;

namespace UserContextMenuApp
{
    public static partial class Util
    {
        public static class UI
        {
            public static Color ToColor(FrameworkElement e, string light, string dark)
            {
                return IsDarkThemed(e) ? dark.ToColor() : light.ToColor();
            }

            public static void ToggleTheme(Window window)
            {
                var rootElement = window.Content as FrameworkElement;
                rootElement.RequestedTheme = IsDarkThemed(rootElement) ? ElementTheme.Light : ElementTheme.Dark;
            }

            public static bool IsDarkThemed(FrameworkElement e)
            {
                if (e.RequestedTheme != ElementTheme.Default)
                    return e.RequestedTheme == ElementTheme.Dark;
                return Application.Current.RequestedTheme == ApplicationTheme.Dark;
            }

            public static nint GetWindowHandle(Window window)
            {
                return WinRT.Interop.WindowNative.GetWindowHandle(window);
            }

            public static nint GetWindowHandle(UIElement element)
            {
                return (nint)element.XamlRoot.ContentIslandEnvironment.AppWindowId.Value;
            }

            public static void SetWindowTitleBar(Window window, TitleBar titleBar)
            {
                var appTitleBar = window.AppWindow.TitleBar;
                var rootElement = window.Content as FrameworkElement;

                // Replace the system title bar.
                window.SetTitleBar(titleBar);
                window.ExtendsContentIntoTitleBar = true;

                appTitleBar.PreferredHeightOption = TitleBarHeightOption.Tall;

                void OnActualThemeChanged(FrameworkElement e, Object args)
                {
                    rootElement.DispatcherQueue.TryEnqueue(() =>
                    {
                        appTitleBar.ButtonForegroundColor = ToColor(rootElement, "Black", "White");
                        appTitleBar.ButtonHoverForegroundColor = appTitleBar.ButtonForegroundColor;
                        appTitleBar.ButtonPressedForegroundColor = appTitleBar.ButtonForegroundColor;
                        appTitleBar.ButtonHoverBackgroundColor = ToColor(rootElement, "#1E000000", "#C8191919");
                        appTitleBar.ButtonPressedBackgroundColor = ToColor(rootElement, "#3C000000", "#64191919");
                    });
                }

                OnActualThemeChanged(null, null);
                rootElement.ActualThemeChanged += OnActualThemeChanged;
            }
        }
    }
}
