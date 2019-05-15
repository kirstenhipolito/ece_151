#!/usr/bin/python

#Kirsten Rae C. Hipolito
#ECE 151 MP2

import sys
import socket
import array
from scapy.all import *
from collections import Counter
from geoip import geolite2
from beautifultable import BeautifulTable
import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.basemap import Basemap
import image
from gmplot import gmplot

if len(sys.argv) > 2:
    print("Usage: ./hipolito_mp2.py [time period for monitoring]")
    sys.exit()

#this function was copied from https://stackoverflow.com/questions/166506/finding-local-ip-addresses-using-pythons-stdlib
#it gets the local host IP address so that won't be included in parsing results anymore
def get_ip():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        # doesn't even have to be reachable
        s.connect(('10.255.255.255', 1))
        IP = s.getsockname()[0]
    except:
        IP = '127.0.0.1'
    finally:
        s.close()
    return IP
#end of get_IP code

#make a list of TCP packets
listofIPs = [[],[]]
localIP = get_ip()  #get local host's IP to avoid counting it in
if (len(sys.argv) == 2):
    pkts = sniff(timeout=int(sys.argv[1]), filter="ip")     #set sniffing on
else:
    pkts = sniff(timeout=60, filter="ip")                   #if no additional argument provided, set sniffing on with default timeout = 60s
#from packets in sniffing results, get TCP packets
for i in range(len(pkts)):
    if ((TCP in pkts[i]) and (localIP != pkts[i][IP].src)):
        listofIPs[0].append(pkts[i][IP].src)
        listofIPs[1].append(pkts[i][IP].len)
    elif ((TCP in pkts[i]) and (localIP != pkts[i][IP].dst)):
        listofIPs[0].append(pkts[i][IP].dst)
        listofIPs[1].append(pkts[i][IP].len)

#from the list of IPs were obtained, get all the unique entries, and tabulate the number of instances
uniqueIPs = [Counter(listofIPs[0]).keys(), Counter(listofIPs[0]).values(), Counter(listofIPs[0]).values()]

#find the indices of each unique IP in the original filtered TCP list, then get the total the lengths of packets exchanged with that IP
for i in range(len(uniqueIPs[0])):
    lenlistind = [j for j,x in enumerate(listofIPs[0]) if x == uniqueIPs[0][i]]     #gets the indices of the packets associated with that IP
    for k in lenlistind:
        uniqueIPs[1][i] += listofIPs[1][k]  #gets the sum of the packet lengths of above packets

#instantiate a table for neatly organizing the data to be collected
IPtable = BeautifulTable()
IPtable.column_headers = ["IP addr", "Total bytes of traffic", "Total TCP packets", "Domain Name", "Country", "Location"]
#get host name from IP, country and lat/long from GeoIP, then print out collected information
for i in range(len(uniqueIPs[0])):
    try:
        p = socket.gethostbyaddr(uniqueIPs[0][i])   #try to get the domain name by reverse DNS lookup
    except socket.herror:
        p = ["Domain not found", 0]

    matchIPaddr = geolite2.lookup(uniqueIPs[0][i])  #try to get geoIP data from GeoLite for each IP

    #store the data in a row of IPtable
    if matchIPaddr is not None:
        IPtable.append_row([uniqueIPs[0][i], uniqueIPs[1][i], uniqueIPs[2][i], p[0], matchIPaddr.country, matchIPaddr.location])
    else:
        IPtable.append_row([uniqueIPs[0][i], uniqueIPs[1][i], uniqueIPs[2][i], p[0], 'Not in GeoLite database', 'Not in GeoLite database'])

#print collected data
print(IPtable)

#tabulate the latitude and longitude into vectors x and y
lat_values = []
lon_values = []
for i in range(len(IPtable)):
    lat_values.append(IPtable[i][5][0])
    lon_values.append(IPtable[i][5][1])

#global projection code from https://makersportal.com/blog/2018/7/20/geographic-mapping-from-a-csv-file-using-python-and-basemap
#slightly changed to display my preferred projection and my data
center_lat = np.mean(lat_values)
center_lon = np.mean(lon_values)
zoom = 90

# input desired coordinates
center = [center_lat,center_lon]

# Setup the bounding box for the zoom and bounds of the map
bbox = [center[0]-zoom,center[0]+zoom,\
        center[1]-zoom,center[1]+zoom]

plt.figure(figsize=(8,12))
# Define the projection, scale, the corners of the map, and the resolution.
m = Basemap(projection='cyl',llcrnrlat=-90,urcrnrlat=90,\
            llcrnrlon=-180,urcrnrlon=180,lat_ts=10,resolution='i')

# Draw coastlines and fill continents and water with color
m.drawcoastlines()
m.fillcontinents(color='peru',lake_color='dodgerblue')

# draw parallels, meridians, and color boundaries
m.drawparallels(np.arange(bbox[0],bbox[1],(bbox[1]-bbox[0])/5),labels=[1,0,0,0])
m.drawmeridians(np.arange(bbox[2],bbox[3],(bbox[3]-bbox[2])/5),labels=[0,0,0,1],rotation=45)
m.drawmapboundary(fill_color='dodgerblue')

# build and plot coordinates onto map
x,y = m(lon_values,lat_values)

for i in range(len(x)):
    m.plot(x[i],y[i],marker='D',color='r')

#title and save plot
plt.title("TCP Packet Location Map")
plt.savefig('packet_location_map.png', format='png', dpi=500)
plt.show()
