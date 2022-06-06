using System.Drawing;
using System.Drawing.Imaging;
using FFMediaToolkit;
using FFMediaToolkit.Decoding;
using FFMediaToolkit.Graphics;

namespace Filter;
static class FileWriter
{
    static Thread videoPlayThread = null;
    static bool videoTerminate = true;
    static int width = 1920;
    static int height = 1080;
    static int frameRate = 30;
    static PPlusVideoStream stream = null;

    static String[] videoExtensions =
    {
        ".mp4", ".avi", ".mkv"
    };

    static String[] imageExtensions =
    {
        ".png", "jpg", "jpeg", "bmp"
    };

    static void Main(String[] args)
    {
        Console.WriteLine("setting FFmpegPath");
        FFmpegLoader.FFmpegPath = @"C:\Program Files\FFmpeg\bin";
        Console.WriteLine("FFmpegPath set");
        stream = new PPlusVideoStream(
            width: width,
            height: height,
            frameRate: frameRate
        );
        Console.WriteLine("stream created");
        while (true) {
            String input = Console.ReadLine();
            if (String.IsNullOrEmpty(input)) { continue; }
            input = input.Trim();
            try
            {
                if (input.Equals("q")) {
                    Environment.Exit(0);
                } else if (isVideoPath(input))
                {
                    stopVideoPlay();
                    videoPlayThread = new Thread(() => playVideoFile(input) );
                    videoTerminate = false;
                    videoPlayThread.Start();
                } else if (isImagePath(input))
                {
                    stopVideoPlay();
                    Bitmap bitmap = new Bitmap(input);
                    stream.pushFrame(bitmap);
                } else
                {
                    Console.WriteLine("invalid instruction");
                }
            }catch(Exception e)
            {
                Console.WriteLine(e.Message);
            }
        }
    }

    static bool isImagePath(String path)
    {
        for (int i = 0; i < imageExtensions.Length; i++)
        {
            if (path.EndsWith(imageExtensions[i])) {
                return true;
            }
        }
        return false;
    }

    static bool isVideoPath(String path)
    {
        for (int i = 0; i < videoExtensions.Length; i++) {
            if (path.EndsWith(videoExtensions[i])) {
                return true;
            }
        }
        return false;
    }

    static unsafe void playVideoFile(String videoPath)
    {
        MediaFile file = MediaFile.Open(videoPath);
        int width = file.Video.Info.FrameSize.Width;
        int height = file.Video.Info.FrameSize.Height;
        int byteSize = width * height * 3;
        double frameRate = file.Video.Info.AvgFrameRate;
        int frameLength = (int) (1000 / frameRate);
        Bitmap bitmap = new Bitmap(width, height, PixelFormat.Format32bppArgb);
        Rectangle rect = new Rectangle(0, 0, width, height);
        while(file.Video.TryGetNextFrame(out var imageData))
        {
            //byte[] imageDataArray = imageData.Data.ToArray();
            BitmapData data = bitmap.LockBits(
                rect, ImageLockMode.WriteOnly, PixelFormat.Format24bppRgb
            );
            fixed (byte *p = imageData.Data)
            {
                IntPtr intPtr = new IntPtr(p);
                Buffer.MemoryCopy((void*)intPtr, (void*)data.Scan0, byteSize, byteSize);
                //System.Runtime.InteropServices.Marshal.Copy(intPtr, 0, data.Scan0, imageDataArray.Length);
            }
            
            
            bitmap.UnlockBits(data);
            stream.pushFrame(bitmap);
            if (videoTerminate) { break; }
            Thread.Sleep(frameLength);
        }
    }

    static void stopVideoPlay()
    {
        videoTerminate = true;
        if (videoPlayThread != null)
        {
            videoPlayThread.Join();
            videoPlayThread = null;
        }
    }
}