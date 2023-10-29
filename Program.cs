
using System.Runtime.InteropServices;

namespace FreeTrack{
    static class Program{
        [DllImport("Test")]
        public static extern int allocCamera(string path);
        static void Main()
        {
            allocCamera("/dev/media0");
        }
    }
}