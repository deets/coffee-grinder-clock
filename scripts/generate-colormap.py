from matplotlib import cm

#j = cm.jet
#j = cm.gnuplot
j = cm.brg

for i in range(j.N):
    r, g, b, a = [int(v * 255) for v in j(i)]
    print(f"0x{r:02x}{g:02x}{b:02x},")
