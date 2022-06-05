using System.Drawing;
using System.IO.MemoryMappedFiles;
using System.Drawing.Imaging;
using System.Collections.Concurrent;
using Microsoft.MixedReality.WebRTC;

namespace Filter {
    public class PPlusVideoStream {
        int height;
        int width;
        int frameByteSize;
        int frameRate;
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
        ImageConverter converter = new ImageConverter();
        public PPlusVideoStream(
            String mmfName = "PPlusCameraSharedBuffer",
            String mmfSemaphoreName = "Global\\PPlusVideoFrameSemaphore",
            int height = 1080,
            int width = 1920,
            int frameRate = 30)
        {
            this.height = height;
            this.width = width;
            this.frameByteSize = height * width * 4;
            this.frameRate = frameRate;
            this.frameDuration = 1000 / frameRate;
            this.currentFrame = new Bitmap(width, height, PixelFormat.Format32bppRgb);
            this.rect = new Rectangle(0, 0, width, height);
            this.frameWarehouse = new FrameWarehouse(3, frameByteSize, frameQueue);
            int imageSize = frameByteSize;
            videoSemaphore = new Semaphore(0, 1, mmfSemaphoreName);
            mmf = MemoryMappedFile.CreateOrOpen(mmfName, imageSize);
            videoAccessor = mmf.CreateViewAccessor();
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
            //IntPtr nextFrameSource = nextFrame.data;
            //if((nextFrame.height != this.height)
            //    || (nextFrame.width != this.width)){
            //    uint sourceDataSize = nextFrame.width * nextFrame.height * 4;
            //    byte[] sourceData = new byte[sourceDataSize];
            //    System.Runtime.InteropServices.Marshal.Copy(nextFrameSource, sourceData, 0, (int)sourceDataSize);
            //    Bitmap resizedImage = resizeFrame(nextFrame.data, (int)nextFrame.width, (int)nextFrame.height);
            //    pushFrame(resizedImage);
            //    return;
            //    //centerFrame(nextFrame.data, (int)nextFrame.width, (int)nextFrame.height, nextFrameData);
            //} else {
            //    System.Runtime.InteropServices.Marshal.Copy(nextFrame.data, nextFrameData, 0, frameByteSize);
            //}
            System.Runtime.InteropServices.Marshal.Copy(nextFrame.data, nextFrameData, 0, frameByteSize);
            frameQueue.Enqueue(nextFrameData);
        }
        //static byte[] resizeBuffer = null;
        //Bitmap resizeBMP = null;
        //Rectangle resizeRect;
        //private Bitmap resizeFrame(IntPtr frameData, int width, int height)
        //{
            //int currentFrameByteSize = width * height * 4;
            //if ((resizeBuffer == null)
            //    || (resizeBuffer.Length != currentFrameByteSize)) {
            //    resizeBuffer = new byte[currentFrameByteSize];
            //    resizeBMP = new Bitmap(width, height);
            //    resizeRect = new Rectangle(0, 0, width, height);
            //}
            //System.Runtime.InteropServices.Marshal.Copy(frameData, resizeBuffer, 0, currentFrameByteSize);

            ////int newWidth = this.height / height * width;
            ////int newHeight = this.width / width * height;
            ////if (newWidth > this.width) {
            ////    newWidth = this.width;
            ////} else {
            ////    newHeight = this.height;
            ////}
            //BitmapData resizeBMPData = resizeBMP.LockBits(
            //    resizeRect, ImageLockMode.ReadWrite, PixelFormat.Format32bppArgb
            //);
            //System.Runtime.InteropServices.Marshal.Copy(resizeBuffer, 0, resizeBMPData.Scan0, currentFrameByteSize);
            //resizeBMP.UnlockBits(resizeBMPData);
            //Bitmap newImage = new Bitmap(resizeBMP, new Size(this.width, this.height));

            //return newImage;

        //    MemoryStream memoryStream = new MemoryStream(frameData);
        //}

        //private void centerFrame(IntPtr frameData, int width, int height, byte[] frame)
        //{
        //    int vPad = (this.height - height) / 2;
        //    int hPad = (this.width - width) / 2;
        //    int framePos = (vPad * this.width + hPad)*4;
        //    IntPtr frameRowPtr = frameData;
        //    for(int row = 0; row < height; row++) {
        //        System.Runtime.InteropServices.Marshal.Copy(frameRowPtr, frame, framePos, width * 4);
        //        frameRowPtr = IntPtr.Add(frameRowPtr, width);
        //        framePos += this.width * 4;
        //    }
        //}

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

                Thread.Sleep(frameRate);
            }
        }

        void fresh(byte[] nextFrame)
        {
            videoSemaphore.WaitOne(0);
            videoAccessor.WriteArray(0, nextFrame, 0, nextFrame.Length);
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
