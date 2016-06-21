#!/usr/bin/python
# -*- coding: utf-8 -*-

import sqlite3 as lite
import sys

con = lite.connect('test.db')


# "Store","Dept","WeekDate","WeeklySales","IsHoliday","Type","Size","Temperature","FuelPrice","CPI","UnemploymentRate"

sales = """create table if not exists Sales(
		Dept			INTEGER,
		Store			INTEGER,
		WeekDate		DATE,
		WeeklySales 	REAL,
		IsHoliday		BOOLEAN,
		Type			CHAR(1),
		Size 			INTEGER,
		Temperature 	REAL,
		CPI				REAL,	
		FuelPrice		REAL,
		UnemploymentRate	REAL,
		PRIMARY KEY(Dept, Store, WeekDate)
	)"""

sales1 = """create table if not exists Sales1(
		Dept	INTEGER,
		Store	INTEGER,	
		WeekDate	DATE,
		WeeklySales	REAL,
		PRIMARY KEY (Dept, Store, WeekDate)
		FOREIGN KEY (Dept, Store, WeekDate) REFERENCES Sales(Dept, Store, WeekDate)
	)"""

sales2 = """create table if not exists Sales2(
		Store	INTEGER,
		Type	CHAR(1),
		Size	INTEGER,
		PRIMARY KEY (Store)
		FOREIGN KEY (Store) REFERENCES Sales(Store)
	)"""

sales3 = """create table if not exists Sales3(
		Store			INTEGER,
		WeekDate		DATE,
		CPI				REAL,	
		FuelPrice		REAL,
		Temperature		REAL,
		UnemploymentRate	REAL,
		PRIMARY KEY(Store, WeekDate)
		FOREIGN KEY (WeekDate) REFERENCES Sales(WeekDate)
		FOREIGN KEY (Store) REFERENCES Sales(Store)
	)"""

sales4 = """create table if not exists Sales4(
		WeekDate	DATE,	
		IsHoliday	BOOLEAN,
		PRIMARY KEY (WeekDate),
		FOREIGN KEY (WeekDate) REFERENCES Sales(WeekDate)
	)"""

# run it 

with con:
    
    cur = con.cursor()
    cur.execute(sales)
    cur.execute(sales1)
    cur.execute(sales2)
    cur.execute(sales3)
    cur.execute(sales4)

    print "Import Complete"

    con.commit();
   # con.close();

