CREATE TABLE Sales1(
	Dept	INTEGER NOT NULL,
	Store	INTEGER NOT NULL,	
	WeekDate	DATE NOT NULL,
	WeeklySales	REAL NOT NULL,
	PRIMARY KEY (Dept, Store, WeekDate)
)

CREATE TABLE Sales2(
	Store	INTEGER,
	Type	CHAR(1),
	Size	INTEGER,
	PRIMARY KEY (Store)
	FOREIGN KEY (Store) REFERENCES Sales1(Store)
)

CREATE TABLE Sales3(
	Store			INTEGER,
	WeekDate		DATE,
	CPI			REAL,	
	FuelPrice		REAL,
	Temperature		REAL,
	UnemploymentRate	REAL,
	PRIMARY KEY(Store, WeekDate)
	FOREIGN KEY (WeekDate) REFERENCES Sales1(WeekDate)
	FOREIGN KEY (Store) REFERENCES Sales1(Store)
)	

CREATE TABLE Sales4(
	WeekDate	DATE NOT NULL,	
	IsHoliday	BOOLEAN
	PRIMARY KEY (WeekDate)
	FOREIGN (WeekDate) REFERENCES Sales1(WeekDate)
)
	