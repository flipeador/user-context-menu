using CommunityToolkit.WinUI.Helpers;
using Microsoft.UI.Windowing;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Threading.Tasks;
using Windows.Storage;
using Windows.Storage.Pickers;
using Windows.UI;

namespace UserContextMenuApp
{
    public static partial class Util
    {
        public static IEnumerable<T> Filter<T>(string query, params IEnumerable<T>[] args)
        {
            if (args == null) yield break;

            foreach (var items in args)
                foreach (var item in items)
                    if (item.ToString().StartsWith(query, StringComparison.OrdinalIgnoreCase))
                        yield return item;
        }

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

            void OnActualThemeChanged(FrameworkElement e, object args)
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

        public static async Task<StorageFile> PickFile(
            Control owner,
            IEnumerable<string> filters = null,
            PickerViewMode viewMode = PickerViewMode.List)
        {
            owner.IsEnabled = false;
            var hWnd = Util.GetWindowHandle(owner);
            var picker = new FileOpenPicker { ViewMode = viewMode };
            WinRT.Interop.InitializeWithWindow.Initialize(picker, hWnd);
            if (filters == null) picker.FileTypeFilter.Add("*");
            else foreach (var f in filters) picker.FileTypeFilter.Add(f);
            var file = await picker.PickSingleFileAsync();
            owner.IsEnabled = true;
            return file;
        }

        public static async Task<StorageFolder> PickFolder(Control owner)
        {
            owner.IsEnabled = false;
            var hWnd = Util.GetWindowHandle(owner);
            var picker = new FolderPicker { FileTypeFilter = { "*" } };
            WinRT.Interop.InitializeWithWindow.Initialize(picker, hWnd);
            var folder = await picker.PickSingleFolderAsync();
            owner.IsEnabled = true;
            return folder;
        }

        public static (string, int)? PickIcon(Control owner, string path, int index)
        {
            owner.IsEnabled = false;
            var hWnd = Util.GetWindowHandle(owner);
            var icon = UserContextMenuVerb.PickIcon(hWnd, path, index);
            owner.IsEnabled = true;
            return icon;
        }

        public static bool AddRegistryValueElevated(string key, string name, string value, string type)
        {
            var psi = new ProcessStartInfo
            {
                FileName = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.System), "cmd.exe"),
                Arguments = $"/c reg add \"{key}\" /v \"{name}\" /t {type} /d \"{value}\" /f",
                Verb = "runas",
                UseShellExecute = true,
                WindowStyle = ProcessWindowStyle.Hidden
            };

            try
            {
                if (Process.Start(psi) is Process process)
                {
                    process.WaitForExit();
                    return process.ExitCode == 0;
                }
            }
            catch { }

            return false;
        }
    }
}
