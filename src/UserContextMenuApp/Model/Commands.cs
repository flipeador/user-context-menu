using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Text;
using System.Text.Json;
using System.Text.Json.Serialization;
using Microsoft.UI.Xaml;
using UserContextMenuApp.Common;

namespace UserContextMenuApp.Model
{
    public enum CommandState
    {
        Enabled  = 0,
        Disabled = 1,
        Hidden   = 2,
    }

    public enum CommandFlags
    {
        Command    = 0,
        Subcommand = 1,
        Separator  = 8,
    }

    public enum CommandShowCmd
    {
        Hide      = 0,
        Normal    = 1,
        Minimized = 2,
        Maximized = 3,
    }

    [Flags]
    public enum CommandType
    {
        Default    = 0x00000025,
        // Files
        File       = 0x00000001,
        Drive      = 0x00000002,
        Directory  = 0x00000004,
        // Background
        Unknown    = 0x00000008,
        Desktop    = 0x00000010,
        Background = 0x00000020,
    }

    public class CommandItemRegEx
    {
        public string Include { get; set; } = "";
        public string Exclude { get; set; } = "";
    }

    public class CommandItemMulti
    {
        public string Mode { get; set; } = "Off";
        public string Args { get; set; } = "";
    }

    public partial class CommandItem : BindableBase
    {
        private string m_title = "My Command";

        public string Title
        {
            get => m_title;
            set => SetProperty(ref m_title, value);
        }

        [JsonIgnore]
        public string Icon
        {
            get
            {
                int i = Application.Current.RequestedTheme == ApplicationTheme.Dark ? 1 : 0;
                return $"{Icons[i][0]}\t{Icons[i][1]}";
            }
        }

        public IList<IList<Object>> Icons { get; set; } = [
            ["%:INSTALL%/Assets/icon.theme-light.png", 0],
            ["%:INSTALL%/Assets/icon.theme-dark.png", 0],
        ];
        public CommandState State { get; set; } = CommandState.Enabled;
        public CommandFlags Flags { get; set; } = CommandFlags.Command;
        public string File { get; set; } = "";
        public string Verb { get; set; } = "Open";
        public CommandShowCmd Scmd { get; set; } = CommandShowCmd.Normal;
        public string Wdir { get; set; } = "";
        public string Args { get; set; } = "";
        public CommandType Type { get; set; } = CommandType.Default;
        public CommandItemRegEx Regex { get; set; } = new();
        public CommandItemMulti Multi { get; set; } = new();
        public ObservableCollection<CommandItem> Children { get; set; } = [];
    }

    public class CommandViewModel
    {
        private static readonly string s_path = $"{App.s_localFolderPath}/commands.json";

        public ObservableCollection<CommandItem> ItemsSource { get; set; } = [];

        public void Initialize()
        {
            ItemsSource.Clear();

            try
            {
                var text = File.ReadAllText(s_path, Encoding.UTF8);
                var options = new JsonSerializerOptions { PropertyNameCaseInsensitive = true };
                var json = JsonSerializer.Deserialize<IEnumerable<CommandItem>>(text, options);
                foreach (var item in json) ItemsSource.Add(item);
            }
            catch { }

            if (ItemsSource.Count == 0)
                ItemsSource.Add(new());
        }

        public void Save()
        {
            var options = new JsonSerializerOptions
            {
                WriteIndented = true,
                PropertyNamingPolicy = JsonNamingPolicy.CamelCase
            };
            var json = JsonSerializer.Serialize(ItemsSource, options);
            File.WriteAllText(s_path, json, Encoding.UTF8);
        }
    }
}
