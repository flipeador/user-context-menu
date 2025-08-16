using System;
using System.IO;
using System.Reflection.Emit;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media.Imaging;
using Windows.Storage.Streams;

namespace UserContextMenuApp.Controls
{
    public sealed partial class CommandIcon : UserControl
    {
        public DependencyProperty SourceProperty =
            DependencyProperty.Register(nameof(Label), typeof(string), typeof(CommandIcon),
                new PropertyMetadata(default(string), new PropertyChangedCallback(OnSourceChanged)));

        public CommandIcon()
        {
            InitializeComponent();
        }

        public void SetImage(object source)
        {
            var bitmapImage = new BitmapImage();
            if (source is string uri)
                bitmapImage.UriSource = new Uri(uri);
            else if (source is IRandomAccessStream stream)
                bitmapImage.SetSource(stream);
            Content = new Image { Source = bitmapImage, Width = 20, Height = 20 };
        }

        public string Source
        {
            get => (string)GetValue(SourceProperty);
            set => SetValue(SourceProperty, value);
        }

        private static void OnSourceChanged(DependencyObject obj, DependencyPropertyChangedEventArgs args)
        {
            var self = (CommandIcon)obj;
            var source = ((string)args.NewValue).Split('\t');

            var path = UserContextMenuVerb.FindPath(source[0]);
            var hIcon = UserContextMenuVerb.ExtractIcon(path, int.Parse(source[1]));

            try
            {
                if (hIcon != nint.Zero)
                {
                    using var icon = System.Drawing.Icon.FromHandle(hIcon);
                    var stream = new MemoryStream();
                    using var bitmap = icon.ToBitmap();
                    bitmap.Save(stream, System.Drawing.Imaging.ImageFormat.Png);
                    stream.Position = 0;
                    using var stream2 = stream.AsRandomAccessStream();
                    self.SetImage(stream2);
                    return;
                }
                else if (path.Contains('.'))
                {
                    self.SetImage(path);
                    return;
                }
            }
            catch { }

            self.Content = new FontIcon { Glyph = "\xE8A4", Width = 20, Height = 20 };
        }
    }
}
