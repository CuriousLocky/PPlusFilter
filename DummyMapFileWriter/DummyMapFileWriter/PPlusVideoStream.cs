using System.Drawing;
using System.IO.MemoryMappedFiles;
using System.Drawing.Imaging;
using System.Collections.Concurrent;
using Microsoft.MixedReality.WebRTC;

namespace Filter {
    public class PPlusVideoStream {
        int frameByteSize;
        int frameDuration;
        class FrameWarehouse {
            int frameByteSize;
            ConcurrentQueue<byte[]> warehouse;
            ConcurrentQueue<byte[]> frameQueue;
            public FrameWarehouse(int size, int frameByteSize, ConcurrentQueue<byte[]> frameQueue)
            {
                this.frameQueue = frameQueue;
                this.frameByteSize = frameByteSize;
                warehouse = new ConcurrentQueue<byte[]>();
                for (int i = 0; i < size; i++) {
                    warehouse.Enqueue(new byte[frameByteSize]);
                }
            }
            public void store(byte[] frame)
            {
                warehouse.Enqueue(frame);
            }

            public byte[] get()
            {
                byte[] result = null;
                if(frameQueue.Count > 10) {
                    if(frameQueue.TryDequeue(out result)) {
                        return result;
                    }
                }
                
                if (warehouse.TryDequeue(out result)) {
                    return result;
                }
                return new byte[frameByteSize];
            }
        }
        Semaphore videoSemaphore;
        Thread videoThread;
        FrameWarehouse frameWarehouse;
        ConcurrentQueue<byte[]> frameQueue = new ConcurrentQueue<byte[]>();
        MemoryMappedFile mmf;
        MemoryMappedViewAccessor videoAccessor;
        bool videoThreadTerminate = false;
        Bitmap currentFrame;
        Rectangle rect;
        public PPlusVideoStream(
            String mmfName = "PPlusCameraSharedBuffer",
            String mmfSemaphoreName = "Global\\PPlusVideoFrameSemaphore",
            int height = 1080,
            int width = 1920,
            int frameRate = 30)
        {
            this.frameByteSize = height * width * 4;
            this.frameDuration = 1000 / frameRate;
            this.currentFrame = new Bitmap(width, height, PixelFormat.Format32bppRgb);
            this.rect = new Rectangle(0, 0, width, height);
            this.frameWarehouse = new FrameWarehouse(3, frameByteSize, frameQueue);
            int imageSize = frameByteSize;
            videoSemaphore = new Semaphore(0, 1, mmfSemaphoreName);
            mmf = MemoryMappedFile.CreateOrOpen(mmfName, 16 + frameByteSize);
            videoAccessor = mmf.CreateViewAccessor();
            videoAccessor.Write(0, width);
            videoAccessor.Write(4, height);
            videoAccessor.Write(8, frameRate);
            videoThread = new Thread(() => execute());
            videoThread.Start();
        }

        ~PPlusVideoStream()
        {
            videoThreadTerminate = true;
            if (videoThread != null) {
                videoThread.Join();
            }
        }

        public void pushFrame(Argb32VideoFrame nextFrame)
        {
            byte[] nextFrameData = frameWarehouse.get();
            System.Runtime.InteropServices.Marshal.Copy(nextFrame.data, nextFrameData, 0, frameByteSize);
            frameQueue.Enqueue(nextFrameData);
        }

        public void pushFrame(Bitmap nextFrameBitmap)
        {
            byte[] nextFrameData = frameWarehouse.get();
            using (Graphics g = Graphics.FromImage(currentFrame)) {
                g.Clear(Color.White);
                g.DrawImage(nextFrameBitmap, Point.Empty);
            }
            BitmapData bitmapData = currentFrame.LockBits(
                rect, ImageLockMode.ReadWrite,
                PixelFormat.Format32bppRgb
            );
            System.Runtime.InteropServices.Marshal.Copy(bitmapData.Scan0, nextFrameData, 0, frameByteSize);
            currentFrame.UnlockBits(bitmapData);
            frameQueue.Enqueue(nextFrameData);
        }

        void execute()
        {
            while (!videoThreadTerminate) {
                byte[] nextFrame = null;
                if (frameQueue.TryDequeue(out nextFrame)) {
                    fresh(nextFrame);
                } else {
                    fresh();
                }

                Thread.Sleep(frameDuration);
            }
        }

        void fresh(byte[] nextFrame)
        {
            videoSemaphore.WaitOne(0);
            videoAccessor.WriteArray(16, nextFrame, 0, nextFrame.Length);
            videoSemaphore.Release(1);
            frameWarehouse.store(nextFrame);
        }

        void fresh()
        {
            videoSemaphore.WaitOne(0);
            videoSemaphore.Release(1);
        }
    }

}
