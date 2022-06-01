using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.IO.MemoryMappedFiles;
using System.Text;
using NAudio.Wave;

class FileWriter {
    const int height = 1080;
    const int width = 1920;
    static Semaphore videoSemaphore;
    static Semaphore audioSemaphore;
    static Bitmap midBitmap = new Bitmap(width, height, PixelFormat.Format24bppRgb);
    static Thread audioThread = null;
    const int audioBufferSize = 16 * 1024;
    static byte[] audioBuffer;
    static MemoryMappedViewAccessor audioAccessor;
    static bool audioThreadTerminate = false;
    static void Main(String[] args)
    {
        int rgbArraySize = height * width * 3;
        videoSemaphore = new Semaphore(0, 1, "Global\\PPlusVideoFrameSemaphore");
        audioSemaphore = new Semaphore(0, 1, "Global\\PPlusAudioSemaphore");
        MemoryMappedFile audioMMF = MemoryMappedFile.CreateOrOpen("PPlusAudioSharedBuffer", audioBufferSize);
        
        MemoryMappedFile videoMMF = MemoryMappedFile.CreateOrOpen("PPlusCameraSharedBuffer", rgbArraySize);
        MemoryMappedViewAccessor accessor = videoMMF.CreateViewAccessor();
        audioAccessor = audioMMF.CreateViewAccessor();
        byte[] buffer = new byte[rgbArraySize];
        audioBuffer = new byte[audioBufferSize];
        //accessor.WriteArray(0, buffer, 0, buffer.Length);
        //Console.WriteLine("Finished writing");
        while (true) {
            String input = Console.ReadLine();
            if (input.Equals("q")) {
                Environment.Exit(0);
            } else if (input.StartsWith("v")){
                parseVideoInput(input.Split(' ')[1], buffer);
            } else if (input.StartsWith("a")) {
                parseAudioInput(input.Split(' ')[1]);
            }
            
            accessor.WriteArray(0, buffer, 0, buffer.Length);
        }
    }

    static void parseAudioInput(String input)
    {
        if ((input == null) || (input.Length == 0)) {
            return;
        } else if (input.Equals("stop")) {
            audioThreadTerminate = true;
            audioThread.Join();
            audioThread = null;
            return;
        }
        audioThreadTerminate = true;
        if (audioThread != null) {
            audioThread.Join();
        }
        audioThread = new Thread( () => playAudio(input));
        audioThreadTerminate = false;
        audioThread.Start();
    }

    static void parseVideoInput(String input, byte[] buffer)
    {
        if ((input == null) || (input.Length == 0)) {
            return;
        } else if (input.Equals("blank")) {
            blank(buffer);
        } else {
            readRGBArray(input, buffer);
        }
    }

    static public void blank(byte[] buffer)
    {
        videoSemaphore.WaitOne(0);
        using (Graphics g = Graphics.FromImage(midBitmap)) {
            g.Clear(Color.White);
        }
        videoSemaphore.Release(1);
    }

    static void playAudio(String audioPath)
    {
        AudioFileReader fileReader = new AudioFileReader(audioPath);
        WaveFormat format = new WaveFormat();
        MediaFoundationResampler resampler = new MediaFoundationResampler(fileReader, format);
        int byteSize = format.ConvertLatencyToByteSize(50);
        Console.WriteLine("50 ms latency equals " + byteSize);
        while (!audioThreadTerminate) {
            Thread.Sleep(50);
            resampler.Read(audioBuffer, 0, byteSize);
            if (resampler.Read(audioBuffer, 0, byteSize) == 0) {
                resampler.Reposition();
                resampler.Read(audioBuffer, 0, byteSize);
            }
            audioSemaphore.WaitOne(0);
            audioAccessor.WriteArray(0, audioBuffer, 0, audioBuffer.Length);
            audioSemaphore.Release(1);
        }
    }

    static void readRGBArray(String filePath, byte[] buffer)
    {
        using (Bitmap bitmap = new Bitmap(filePath)) {
            videoSemaphore.WaitOne(0);

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
            videoSemaphore.Release(1);
        }
    }
}