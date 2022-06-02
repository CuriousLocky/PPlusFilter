using System.Drawing;
using Xabe.FFmpeg;
class FileWriter {
    static void Main(String[] args)
    {
        String mmfName = "PPlusCameraSharedBuffer";
        String semaphoreName = "Global\\PPlusVideoFrameSemaphore";
        PPlusVideoStream stream = new PPlusVideoStream(mmfName, semaphoreName);
        while (true) {
            String input = Console.ReadLine();
            if (input == null) { continue; }
            if (input.Equals("q")) {
                Environment.Exit(0);
            } else {
                try {
                    Bitmap nextFrame = new Bitmap(input);
                    stream.pushFrame(nextFrame);
                } catch (Exception) {
                    Console.WriteLine("Invalid picture path");
                }
            }
        }
    }
}