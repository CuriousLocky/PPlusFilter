using System;
using System.Drawing;
using System.IO;
using System.IO.MemoryMappedFiles;
using System.Text;

class FileWriter{
    const int height = 512;
    const int width = 512;
    static void Main(String[] args) {
        int rgbArraySize = height * width * 3;
        using(MemoryMappedFile mmf = MemoryMappedFile.CreateOrOpen("PPlusCameraSharedBuffer", rgbArraySize)) {
            MemoryMappedViewAccessor accessor = mmf.CreateViewAccessor();
            byte[] buffer = new byte[rgbArraySize];
            //accessor.WriteArray(0, buffer, 0, buffer.Length);
            //Console.WriteLine("Finished writing");
            while (true) {
                String input = Console.ReadLine();
                parseInput(input, buffer);
                accessor.WriteArray(0, buffer, 0, buffer.Length);
            }
        }
    }

    static void parseInput(String input, byte[] buffer){
        if ((input == null) || (input.Length == 0)) {
            return;
        } else if (input.Equals("blank")) {
            blank(buffer);
        } else if (input.Equals("q")){
            Environment.Exit(0);
        } else {
            readRGBArray(input, buffer);
        }
    }

    static void blank(byte[] buffer){
        for (int i = 0; i < buffer.Length; i++) {
            buffer[i] = 0;
        }
    }

    static void readRGBArray(String filePath, byte[] buffer) {
        using(Bitmap bitmap = new Bitmap(filePath)) {
            int rowSize = Math.Min(bitmap.Width, width);
            int colSize = Math.Min(bitmap.Height, height);
        
            Rectangle rect = new Rectangle(0, 0, bitmap.Width, bitmap.Height);
            System.Drawing.Imaging.BitmapData bitmapData =
                bitmap.LockBits(rect, System.Drawing.Imaging.ImageLockMode.ReadWrite,
                bitmap.PixelFormat);

            IntPtr ptr = bitmapData.Scan0;

            int bitmapBytes = Math.Abs(bitmapData.Stride) * bitmap.Height;
            byte[] rgbValues = new byte[bitmapBytes];
        
            System.Runtime.InteropServices.Marshal.Copy(ptr, rgbValues, 0, bitmapBytes);

            for (int i = 0; i < buffer.Length; i++) {
                buffer[i] = 0;
            }

            for (int row = 0; row < colSize; row++) {
                Array.Copy(rgbValues, row * rowSize * 3, buffer, row * width * 3, rowSize * 3);
            }

            bitmap.UnlockBits(bitmapData);
        }
    }
}