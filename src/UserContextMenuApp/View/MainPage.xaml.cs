using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Navigation;
using Microsoft.Web.WebView2.Core;
using System;
using System.Text;
using UserContextMenuApp.Model;
using Windows.Storage;
using Windows.System;
using Windows.Win32.System.DataExchange;

namespace UserContextMenuApp.View
{
    public sealed partial class MainPage : Page
    {
        private MainWindow m_window;

        private readonly CommandViewModel m_commandViewModel = new();
        private readonly PackageViewModel m_packageViewModel = new();

        public MainPage()
        {
            InitializeComponent();
        }

        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            m_window = e.Parameter as MainWindow;
        }

        private void OnPageLoaded(object sender, RoutedEventArgs e)
        {
            LoadCommands(null, null);
            m_window.AddMessageHandler(OnMessage);
        }

        private void OnPageUnloaded(object sender, RoutedEventArgs e)
        {
            m_window.RemoveMessageHandler(OnMessage);
        }

        private unsafe nint? OnMessage(uint msg, nint wParam, nint lParam)
        {
            if (msg == 0x004A) // WM_COPYDATA
            {
                var data = (COPYDATASTRUCT*)lParam;
                var result = Encoding.Unicode.GetString((byte*)data->lpData, (int)data->cbData);
                DispatcherQueue.TryEnqueue(() =>
                {
                    var timestamp = DateTime.Now.ToString("HH:mm:ss.fff");
                    e_debugInfo.Text += $"[{timestamp}] {result}\n";
                });
                return 1;
            }
            return null;
        }

        private void LoadCommands(object sender, RoutedEventArgs args)
        {
            e_commands.SelectedItem = null;
            m_commandViewModel.Initialize();

            if (e_commands.RootNodes.Count > 0)
                e_commands.SelectedNode = e_commands.RootNodes[0];

            m_packageViewModel.Initialize();
        }

        private void AddChildCommand(object sender, RoutedEventArgs args)
        {
            if (e_commands.SelectedNode?.Content is CommandItem command)
                command.Children.Add(new());
        }

        private void RemoveSelectedCommand(object sender, RoutedEventArgs args)
        {
            if (e_commands.SelectedNode?.Content is CommandItem command)
            {
                if (e_commands.SelectedNode?.Parent?.Content is CommandItem parent)
                {
                    e_commands.SelectedItem = null;
                    parent.Children.Remove(command);
                }
            }
        }

        private void SaveCommands(object sender, RoutedEventArgs args)
        {
            if (e_commands.SelectedNode?.Content is CommandItem command)
            {
                // Menu item
                // - Title
                command.Title = e_commandTitle.Text.Trim();
                // - Icons
                command.Icons[0][0] = e_commandLightIcon.Text.Trim();
                command.Icons[0][1] = (int)e_commandLightIconIndex.Value;
                command.Icons[1][0] = e_commandDarkIcon.Text.Trim();
                command.Icons[1][1] = (int)e_commandDarkIconIndex.Value;
                command.OnPropertyChanged("Icon");
                // - State
                command.State = Enum.Parse<CommandState>(e_commandState.SelectedItem.ToString());
                // - Type (Flags)
                command.Flags = Enum.Parse<CommandFlags>(e_commandFlags.SelectedItem.ToString());

                // Command
                // - File
                command.File = e_commandFile.Text.Trim();
                // -- Verb
                command.Verb = e_commandVerb.Text.Trim();
                // -- Show Window (Command)
                command.Scmd = Enum.Parse<CommandShowCmd>(e_commandShowCmd.SelectedItem.ToString());
                // -- Working Directory
                command.Wdir = e_commandWorkDir.Text.Trim();
                // - Arguments
                command.Args = e_commandArgs.Text.Trim();

                // Condition
                // - Match Type
                command.Type = (
                    (e_commandTypeFile.IsChecked       == true ? CommandType.File       : 0) |
                    (e_commandTypeDrive.IsChecked      == true ? CommandType.Drive      : 0) |
                    (e_commandTypeDirectory.IsChecked  == true ? CommandType.Directory  : 0) |
                    (e_commandTypeUnknown.IsChecked    == true ? CommandType.Unknown    : 0) |
                    (e_commandTypeDesktop.IsChecked    == true ? CommandType.Desktop    : 0) |
                    (e_commandTypeBackground.IsChecked == true ? CommandType.Background : 0)
                );
                // - Match Name
                command.Regex.Include = e_commandRegexInclude.Text.Trim();
                command.Regex.Exclude = e_commandRegexExclude.Text.Trim();
                // - Multi Mode
                command.Multi.Mode = e_commandMultiMode.SelectedItem.ToString();
                // -- Arguments
                command.Multi.Args = e_commandMultiArgs.Text.Trim();
            }

            m_commandViewModel.Save();
        }

        private void Commands_SelectionChanged(TreeView sender, TreeViewSelectionChangedEventArgs args)
        {
            e_commandTab.IsEnabled = false;

            if (sender.SelectedNode?.Content is CommandItem command)
            {
                // Menu item
                // - Title
                e_commandTitle.Text = command.Title;
                // - Icons
                e_commandLightIcon.Text = command.Icons[0][0].ToString();
                e_commandLightIconIndex.Text = command.Icons[0][1].ToString();
                e_commandDarkIcon.Text = command.Icons[1][0].ToString();
                e_commandDarkIconIndex.Text = command.Icons[1][1].ToString();
                // - State
                e_commandState.SelectedItem = command.State.ToString();
                // - Type (Flags)
                e_commandFlags.SelectedItem = command.Flags.ToString();

                // Command
                // - File
                e_commandFile.Text = command.File;
                // -- Verb
                e_commandVerb.SelectedIndex = -1;
                e_commandVerb.Text = " ";
                e_commandVerb.Text = command.Verb;
                // -- Show Window (Command)
                e_commandShowCmd.SelectedItem = command.Scmd.ToString();
                // -- Working Directory
                e_commandWorkDir.Text = command.Wdir;
                // - Arguments
                e_commandArgs.Text = command.Args;

                // Condition
                // - Match Type
                e_commandTypeFile.IsChecked = command.Type.HasFlag(CommandType.File);
                e_commandTypeDrive.IsChecked = command.Type.HasFlag(CommandType.Drive);
                e_commandTypeDirectory.IsChecked = command.Type.HasFlag(CommandType.Directory);
                e_commandTypeUnknown.IsChecked = command.Type.HasFlag(CommandType.Unknown);
                e_commandTypeDesktop.IsChecked = command.Type.HasFlag(CommandType.Desktop);
                e_commandTypeBackground.IsChecked = command.Type.HasFlag(CommandType.Background);
                // - Match Name
                e_commandRegexInclude.Text = command.Regex.Include;
                e_commandRegexExclude.Text = command.Regex.Exclude;
                // - Multi Mode
                e_commandMultiMode.SelectedItem = command.Multi.Mode;
                // -- Arguments
                e_commandMultiArgs.Text = command.Multi.Args;

                e_commandTab.IsEnabled = true;
            }
        }

        private void Packages_Toggle(object sender, RoutedEventArgs e)
        {
            var checkbox = sender as CheckBox;
            var keyPath = checkbox.Tag as string;

            if (!PackageClsid.SetState(checkbox.IsChecked ?? false, keyPath))
                checkbox.IsChecked = !checkbox.IsChecked;
        }

        private void LaunchLocalFolder(object sender, RoutedEventArgs args)
        {
            _ = Launcher.LaunchFolderAsync(ApplicationData.Current.LocalFolder);
        }

        private void Expander_Loaded_Collapse(object sender, RoutedEventArgs args)
        {
            var expander = sender as Expander;
            expander.IsExpanded = false;
        }

        private void TextBox_KeyDown_SingleLine(object sender, KeyRoutedEventArgs args)
        {
            if (args.Key is VirtualKey.Space or VirtualKey.Enter)
                args.Handled = true;
        }

        private async void Button_Click_PickFile(object sender, RoutedEventArgs args)
        {
            var button = sender as Button;
            var textBox = button.Tag as TextBox;
            var file = await Util.PickFile(button);
            if (file != null) textBox.Text = file.Path;
        }

        private async void Button_Click_PickFolder(object sender, RoutedEventArgs args)
        {
            var button = sender as Button;
            var textBox = button.Tag as TextBox;
            var folder = await Util.PickFolder(button);
            if (folder != null) textBox.Text = folder.Path;
        }

        private void Button_Click_PickIcon(object sender, RoutedEventArgs args)
        {
            var button = sender as Button;
            var textBox = FindName(button.Tag as string) as TextBox;
            var numberBox = FindName($"{button.Tag}Index") as NumberBox;
            var path = UserContextMenuVerb.FindPath(textBox.Text);
            var icon = Util.PickIcon(button, path, (int)numberBox.Value);
            if (icon != null)
            {
                textBox.Text = icon?.Item1;
                numberBox.Value = (double)icon?.Item2;
            }
        }

        private void Pivot_SelectionChanged(object sender, SelectionChangedEventArgs args)
        {
            var selectedPivotItem = m_pivot.SelectedItem as PivotItem;
            if (selectedPivotItem == m_browserPivotItem)
                if (string.IsNullOrEmpty(e_webviewUri.Text))
                    e_webview.Source = new Uri(App.s_repositoryUrl);
        }

        private void Browser_Navigate(object sender, RoutedEventArgs args)
        {
            try
            {
                e_webview.Source = new Uri(e_webviewUri.Text);
            }
            catch { }
        }

        private void Browser_NavigateTo(object sender, RoutedEventArgs args)
        {
            m_pivot.SelectedItem = m_browserPivotItem;
            var element = sender as FrameworkElement;
            var uri = element.Tag.ToString().Trim('/');
            if (uri != e_webview.Source.ToString().Trim('/'))
                e_webview.Source = new Uri(uri);
        }

        private void WebView_NavigationStarting(WebView2 sender, CoreWebView2NavigationStartingEventArgs args)
        {
            e_webviewUri.Text = args.Uri;
        }
    }
}
