using System.Runtime.InteropServices;
using System.Text;

namespace UserContextMenuApp
{
    public partial class UserContextMenuVerb
    {
        private static readonly nint m_module =
            NativeLibrary.Load($"{App.s_installFolderPath}/UserContextMenuVerb.dll");

        private static readonly FindPath_F s_findPath = GetDelegate<FindPath_F>("DllFindPath");
        private static readonly ExtractIcon_F s_extractIcon = GetDelegate<ExtractIcon_F>("DllExtractIcon");

        public static string FindPath(string path)
        {
            var sb = new StringBuilder(32767);
            s_findPath(path, sb);
            return sb.ToString();
        }

        public static nint ExtractIcon(string path, int index)
        {
            return s_extractIcon(path, index);
        }

        private static T GetDelegate<T>(string name)
        {
            return Marshal.GetDelegateForFunctionPointer<T>(NativeLibrary.GetExport(m_module, name));
        }

        [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        private delegate nint FindPath_F(string a, StringBuilder b);

        [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        private delegate nint ExtractIcon_F(string a, int b);
    }
}
