using System;
using System.Collections.Generic;
using System.IO;
using System.Threading.Tasks;
using Microsoft.UI.Xaml.Controls;
using Windows.Storage;
using Windows.Storage.Pickers;

namespace UserContextMenuApp
{
    public static partial class Util
    {
        public static class IO
        {
            public class EnumerateItem
            {
                public string Path { get; set; }
                public string Name { get; set; }
            }

            public static IEnumerable<EnumerateItem> Enumerate(string path, string pattern = "*", EnumerationOptions options = null)
            {
                IEnumerable<string> entries = [];

                path = path.Trim().Trim('"').Trim();

                options ??= new EnumerationOptions
                {
                    MatchType = MatchType.Win32,
                    AttributesToSkip = System.IO.FileAttributes.Hidden
                };

                try
                {
                    if (Path.EndsInDirectorySeparator(path))
                        entries = Directory.EnumerateFileSystemEntries(path, pattern, options);
                    else if (path.Contains('\\') || path.Contains('/'))
                    {
                        var parent = Path.GetDirectoryName(path);
                        if (!String.IsNullOrEmpty(parent))
                        {
                            var name = Path.GetFileName(path);
                            if (!String.IsNullOrEmpty(name))
                            {
                                var invalidNameChars = Path.GetInvalidFileNameChars();
                                if (name.IndexOfAny(invalidNameChars) == -1) name += '*';
                                entries = Directory.EnumerateFileSystemEntries(parent, name, options);
                            }
                        }
                    }
                }
                catch { }

                foreach (var entry in entries)
                {
                    string fullPath, fileName;

                    try
                    {
                        fullPath = Path.GetFullPath(entry);
                        fileName = Path.GetFileName(entry);
                    }
                    catch { continue; }

                    if (!String.IsNullOrEmpty(fullPath) && !String.IsNullOrEmpty(fileName))
                        yield return new EnumerateItem { Path = fullPath, Name = fileName };
                }
            }

            public static async Task<StorageFile> PickFile(
                Control owner,
                IEnumerable<string> filters = null,
                PickerViewMode viewMode = PickerViewMode.List)
            {
                owner.IsEnabled = false;
                var hWnd = Util.UI.GetWindowHandle(owner);
                var picker = new FileOpenPicker { ViewMode = viewMode };
                WinRT.Interop.InitializeWithWindow.Initialize(picker, hWnd);
                if (filters == null)
                    picker.FileTypeFilter.Add("*");
                else foreach (var f in filters)
                        picker.FileTypeFilter.Add(f);
                var file = await picker.PickSingleFileAsync();
                owner.IsEnabled = true;
                return file;
            }

            public static async Task<StorageFolder> PickFolder(Control owner)
            {
                owner.IsEnabled = false;
                var hWnd = Util.UI.GetWindowHandle(owner);
                var picker = new FolderPicker { FileTypeFilter = { "*" } };
                WinRT.Interop.InitializeWithWindow.Initialize(picker, hWnd);
                var folder = await picker.PickSingleFolderAsync();
                owner.IsEnabled = true;
                return folder;
            }
        }
    }
}
