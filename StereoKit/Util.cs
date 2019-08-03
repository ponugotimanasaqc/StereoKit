﻿using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace StereoKit
{
    internal static class NativeLib
    {
        [DllImport("kernel32")]
        static extern IntPtr LoadLibraryEx(string lpFileName, IntPtr hFile, uint dwFlags);
        [DllImport("kernel32")]
        static extern bool FreeLibrary(IntPtr hModule);

        public const string DllName = "StereoKitC.dll";

        static IntPtr library;

        internal static void LoadDll()
        {
            string folder = Environment.Is64BitProcess ?
                "x64_" :
                "Win32_";

            #if DEBUG
            folder += "Debug";
            #else
            folder += "Release";
            #endif

            Console.WriteLine("[StereoKit] Using " + folder + " build.");

            string location = System.Reflection.Assembly.GetExecutingAssembly().Location;
            string path     = Path.Combine(Path.GetDirectoryName(location), folder, DllName);
            library = LoadLibraryEx(path, IntPtr.Zero, 0);

            if (library == IntPtr.Zero)
                throw new Exception("Missing StereoKit DLL, should be at " + path);
        }
        public static void UnloadDLL()
        {
            FreeLibrary(library);
        }
    }
}
