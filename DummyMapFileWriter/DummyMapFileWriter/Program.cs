using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.IO.MemoryMappedFiles;
using System.Text;

class FileWriter {
    const int height = 1080;
    const int width = 1920;
    static Semaphore s;
    static Bitmap midBitmap = new Bitmap(width, height, PixelFormat.Format24bppRgb);
    static void Main(String[] args)
    {
        int rgbArraySize = height * width * 3;
        s = new Semaphore(0, 1, "Global\\PPlusVideoFrameSemaphore");
        using (MemoryMappedFile mmf = MemoryMappedFile.CreateOrOpen("PPlusCameraSharedBuffer", rgbArraySize)) {
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

    static void parseInput(String input, byte[] buffer)
    {
        if ((input == null) || (input.Length == 0)) {
            return;
        } else if (input.Equals("blank")) {
            blank(buffer);
        } else if (input.Equals("q")) {
            Environment.Exit(0);
        } else {
            readRGBArray(input, buffer);
        }
    }

    static void blank(byte[] buffer)
    {
        s.WaitOne(0);
        using (Graphics g = Graphics.FromImage(midBitmap)) {
            g.Clear(Color.White);
        }
        s.Release(1);
    }

    static void readRGBArray(String filePath, byte[] buffer)
    {
        using (Bitmap bitmap = new Bitmap(filePath)) {
            s.WaitOne(0);

            Rectangle rect = new Rectangle(0, 0, width, height);

            using (Graphics g = Graphics.FromImage(midBitmap)) {
                g.Clear(Color.White);
                g.DrawImage(bitmap, Point.Empty);
            }

            System.Drawing.Imaging.BitmapData bitmapData =
                midBitmap.LockBits(rect, System.Drawing.Imaging.ImageLockMode.ReadWrite,
                PixelFormat.Format24bppRgb);

            System.Runtime.InteropServices.Marshal.Copy(bitmapData.Scan0, buffer, 0, width * height * 3);

            midBitmap.UnlockBits(bitmapData);
            s.Release(1);
        }
    }
}