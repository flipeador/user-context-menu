using System.Runtime.InteropServices;

namespace UserContextMenuApp
{
    public partial class UserContextMenuVerb
    {
        private static readonly nint s_module =
            NativeLibrary.Load($"{App.s_installFolderPath}/UserContextMenuVerb.dll");

        private static readonly DllFindPath s_dllFindPath = GetDelegate<DllFindPath>("DllFindPath");
        private static readonly DllPickIcon s_dllPickIcon = GetDelegate<DllPickIcon>("DllPickIcon");
        private static readonly DllExtractIcon s_dllExtractIcon = GetDelegate<DllExtractIcon>("DllExtractIcon");

        [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        private delegate void DllFindPath(string a, out string b);

        [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        private delegate int DllPickIcon(nint a, int b, string c, out string d);

        [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        private delegate nint DllExtractIcon(string a, int b);

        private static T GetDelegate<T>(string name)
        {
            var ptr = NativeLibrary.GetExport(s_module, name);
            return Marshal.GetDelegateForFunctionPointer<T>(ptr);
        }

        public static string FindPath(string path)
        {
            s_dllFindPath(path, out string opath);
            return opath;
        }

        public static (string, int)? PickIcon(nint hWnd, string path, int index)
        {
            index = s_dllPickIcon(hWnd, index, path, out string opath);
            return string.IsNullOrEmpty(opath) ? null : (opath, index);
        }

        public static nint ExtractIcon(string path, int index)
        {
            return s_dllExtractIcon(path, index);
        }
    }
}
