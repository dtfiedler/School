#!/usr/bin/python

import csv
#open file
file = open("SalesFile.csv", "r")
data = {}
indexToName = {}
lineNum = 0
for line in file:
       	if lineNum == 0:
       		i = 0
       		headers = line.split(',')
       		for header in headers:
	       		# headers = line.split(",")[0]
	       		header = header.replace('\"', '')
	       		data[header] = []
	       		indexToName[i] = header
	       		i+=1
     	else:
     		i = 0
     		values = line.split(',')
     		for value in values:
     			value = value.replace('\"', '')
     			data[indexToName[i]] += [value]
     			i += 1
     	lineNum += 1

#Sales1(Dept, Store, WeekDate, WeeklySales)
with open('Sales1.csv', 'w') as fp:
	a = csv.writer(fp)
	a.writerow(('Dept', 'Store', 'WeekDate', 'WeeklySales'))
	i = 0
	for rows in data[indexToName[0]]: 
		a.writerow((data[indexToName[0]][i], data[indexToName[1]][i], data[indexToName[2]][i], data[indexToName[3]][i]))
		i += 1

with open('Sales2.csv', 'w') as fp:
	a = csv.writer(fp)
	a.writerow(('Store', 'Type', 'Size'))
	i = 0
	for rows in data[indexToName[0]]: 
		a.writerow((data[indexToName[0]][i], data[indexToName[5]][i], data[indexToName[6]][i]))
		i += 1

#Sales3(Store, WeekDate, CPI, FuelPrice, Temperature, UnemploymentRate)
with open('Sales3.csv', 'w') as fp:
	a = csv.writer(fp)
	a.writerow(('Store', 'WeekDate', 'FuelPrice', 'Temperature', 'UnemploymentRate'))
	i = 0
	for rows in data[indexToName[0]]: 
		a.writerow((data[indexToName[0]][i], data[indexToName[1]][i], data[indexToName[8]][i], data[indexToName[7]][i], data[indexToName[10]][i]))
		i += 1

#Sales4 (WeekDate, IsHoliday)
with open('Sales4.csv', 'w') as fp:
	a = csv.writer(fp)
	a.writerow(('WeekDate', 'IsHoliday'))
	i = 0
	for rows in data[indexToName[0]]:
		a.writerow((data[indexToName[1]][i], data[indexToName[4]][i]))
		i += 1
file.close()