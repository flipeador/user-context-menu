using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using Windows.Devices.Geolocation;

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
