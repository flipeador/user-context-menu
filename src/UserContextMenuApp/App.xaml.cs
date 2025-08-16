using Microsoft.UI.Xaml;
using Microsoft.Windows.AppNotifications;
using Microsoft.Windows.AppNotifications.Builder;

namespace UserContextMenuApp
{
    public partial class App : Application
    {
        public static readonly string s_localFolderPath = Windows.Storage.ApplicationData.Current.LocalFolder.Path;
        public static readonly string s_installFolderPath = Windows.ApplicationModel.Package.Current.InstalledLocation.Path;
        public static readonly string s_repositoryUrl = "https://github.com/flipeador/user-context-menu";

        private Window m_window;

        public App()
        {
            InitializeComponent();
            UnhandledException += OnUnhandledException;
        }

        protected override void OnLaunched(LaunchActivatedEventArgs args)
        {
            m_window = new MainWindow();
            m_window.Activate();
        }

        private void OnUnhandledException(object sender, UnhandledExceptionEventArgs args)
        {
            #if RELEASE
            var notification = new AppNotificationBuilder()
                .AddText("An exception was thrown.")
                .AddText($"Type: {args.Exception.GetType()}")
                .AddText($"Message: {args.Message}\r\n" +
                         $"HResult: {args.Exception.HResult}")
                .BuildNotification();

            AppNotificationManager.Default.Show(notification);

            args.Handled = true;
            #endif
        }
    }
}
