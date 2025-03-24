import serial  
import mido  
import argparse  

parser = argparse.ArgumentParser(description='发送MIDI文件到串口设备')  
parser.add_argument('port', type=str, help='串口设备路径，例如 /dev/cu.usbserial-69526606B6')  
parser.add_argument('midi_file', type=str, help='MIDI文件路径，例如 /path/to/file.mid')  
args = parser.parse_args()  

port = serial.Serial(args.port, 115200)  
mid = mido.MidiFile(args.midi_file, clip=True)  

for msg in mid.play():  
    b = msg.bin()  
    print(b)  
    port.write(b)
