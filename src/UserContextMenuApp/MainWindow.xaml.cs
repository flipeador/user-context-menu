using System;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Navigation;
using UserContextMenuApp.View;

namespace UserContextMenuApp
{
    public sealed partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();

            Util.UI.SetWindowTitleBar(this, e_titleBar);
        }

        private void Initialize(object sender, RoutedEventArgs e)
        {
            e_rootFrame.NavigationFailed += OnNavigationFailed;
            e_rootFrame.Navigate(typeof(MainPage));
        }

        private void OnNavigationFailed(Object sender, NavigationFailedEventArgs e)
        {
            throw new Exception("Failed to load Page: " + e.SourcePageType.FullName);
        }

        private void ToggleTheme(object sender, RoutedEventArgs e)
        {
            Util.UI.ToggleTheme(this);
        }
    }
}
