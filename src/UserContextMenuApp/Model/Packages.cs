using System;
using System.Collections.ObjectModel;
using System.IO;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.Win32;

namespace UserContextMenuApp.Model
{
    public partial class PackageDataTemplateSelector : DataTemplateSelector
    {
        public DataTemplate PackageItemTemplate { get; set; }
        public DataTemplate PackageClsidTemplate { get; set; }

        protected override DataTemplate SelectTemplateCore(object item)
        {
            return item switch
            {
                PackageItem => PackageItemTemplate,
                PackageClsid => PackageClsidTemplate,
                _ => base.SelectTemplateCore(item)
            };
        }
    }

    public class PackageClsid
    {
        public string Clsid { get; set; }
        public string KeyPath { get; set; }
        public bool IsEnabled { get; set; }

        public PackageClsid(string clsid, string keyPath, bool enabled)
        {
            Clsid = clsid;
            KeyPath = keyPath;
            IsEnabled = enabled;
        }

        public static bool SetState(bool enabled, string keyPath)
        {
            using var clsidKey = Registry.ClassesRoot.OpenSubKey(keyPath);

            if (clsidKey != null && clsidKey.GetValue("DllPath") is string dllPath)
            {
                dllPath = Path.ChangeExtension(dllPath, enabled ? "dll" : "bak");

                try
                {
                    clsidKey.SetValue("DllPath", dllPath);
                    return true;
                }
                catch //(UnauthorizedAccessException)
                {
                    return Util.AddRegistryValueElevated($"HKCR\\{keyPath}", "DllPath", dllPath, "REG_SZ");
                }
            }

            return false;
        }
    }

    public class PackageItem
    {
        public string Text { get; set; }
        public ObservableCollection<PackageClsid> Ids { get; set; }

        public PackageItem(string text, ObservableCollection<PackageClsid> ids)
        {
            Text = text;
            Ids = ids;
        }
    }

    public partial class PackageViewModel
    {
        public ObservableCollection<PackageItem> ItemsSource { get; set; } = [];

        public void Initialize()
        {
            ItemsSource.Clear();

            try
            {
                var rootKeyPath = @"PackagedCom\Package";
                using var rootKey = Registry.ClassesRoot.OpenSubKey(rootKeyPath);
                if (rootKey == null) return;

                foreach (var packageName in rootKey.GetSubKeyNames())
                {
                    var packageIds = new ObservableCollection<PackageClsid>();
                    var packageKeyPath = $@"{rootKeyPath}\{packageName}";

                    try
                    {
                        using var packageKey = Registry.ClassesRoot.OpenSubKey(packageKeyPath);
                        if (packageKey == null) continue;
                        using var classKey = packageKey.OpenSubKey("Class");
                        if (classKey == null) continue;

                        foreach (var clsid in classKey.GetSubKeyNames())
                        {
                            var clsidKeyPath = $@"{packageKeyPath}\Class\{clsid}";
                            using var clsidKey = Registry.ClassesRoot.OpenSubKey(clsidKeyPath);
                            if (clsidKey == null) continue;

                            var threading = clsidKey.GetValue("Threading");

                            if (threading is int threadingValue && threadingValue == 0)
                            {
                                if (clsidKey.GetValue("DllPath") is string dllPath)
                                {
                                    if (!string.IsNullOrEmpty(dllPath))
                                    {
                                        bool enabled = dllPath.EndsWith(".dll");
                                        if (enabled || dllPath.EndsWith(".bak"))
                                            packageIds.Add(new PackageClsid(clsid, clsidKeyPath, enabled));
                                    }
                                }
                            }
                        }
                    }
                    catch { }

                    if (packageIds.Count > 0)
                    {
                        var name = packageName.Split("_")[0];
                        ItemsSource.Add(new PackageItem(name, packageIds));
                    }
                }
            }
            catch { }
        }
    }
}
