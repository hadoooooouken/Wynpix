import subprocess, sys, numpy as np
rec, W, H = sys.argv[1], 240, 135
p = subprocess.run(["ffprobe","-v","error","-select_streams","v","-show_entries","frame=pts_time","-of","csv=p=0",rec],capture_output=True,text=True)
times = [float(x.strip(",")) for x in p.stdout.split() if x.strip(",")]
raw = subprocess.run(["ffmpeg","-v","error","-i",rec,"-fps_mode","passthrough","-vf",f"scale={W}:{H}","-f","rawvideo","-pix_fmt","rgb24","-"],capture_output=True).stdout
n = len(raw)//(W*H*3)
fr = np.frombuffer(raw[:n*W*H*3],np.uint8).reshape(n,H,W,3).astype(int)
r,g,b = fr[...,0],fr[...,1],fr[...,2]
red = ((r>200)&(g<60)&(b<60)).mean(axis=(1,2))
blue = ((b>200)&(r<60)&(g<60)).mean(axis=(1,2))
green = ((g>200)&(r<60)&(b<60)).mean(axis=(1,2))
with open(sys.argv[2],"w") as f:
    for i in range(n):
        f.write(f"{times[i] if i < len(times) else -1:.3f} {red[i]:.3f} {blue[i]:.3f} {green[i]:.3f}\n")
print(n, len(times))
