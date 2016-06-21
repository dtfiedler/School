package jdbcApp;
import java.sql.*;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Random;
import java.util.Scanner;
import java.sql.DriverManager;

public class ConnectJDBC {
	
	static Scanner in = new Scanner(System.in);	
	static List<Integer> rowsToVisit = new ArrayList<Integer>();
	static List<String> tables = new ArrayList<String>();
	
	public static void main (String[] args) throws Exception {
			
		Boolean exit = false;
		do {
			
			String cstring = "";
			Connection con;
			if (args.length > 0){
		         cstring = args[0];
		    } else{
		         cstring = "lab";
		    }
	
		if (cstring.equals("lab")){
	         //Class.forName("org.postgresql.Driver");
             String url = "jdbc:postgresql://stampy.cs.wisc.edu/cs564instr?sslfactory=org.postgresql.ssl.NonValidatingFactory&ssl";
	         con = DriverManager.getConnection(url);
	       	} else {
	         Class.forName("org.postgresql.Driver");
	         con = DriverManager.getConnection("jdbc:postgresql://localhost/hw2", "dtfiedler", "");
	       	}

			DatabaseMetaData meta = con.getMetaData();
			ResultSet tablesData = meta.getTables(null, "public", "%", new String[] {"TABLE"});
			
			System.out.println("************************Available Tables*************************");
			while(tablesData.next()){
				System.out.print(tablesData.getString("TABLE_NAME").toUpperCase() + "  ");
				tables.add(tablesData.getString("TABLE_NAME").toUpperCase());
			}
			
			System.out.println("\nEnter QUERY to create your own query or enter a table name: ");
			String action = in.nextLine().toUpperCase();
			
			if (!action.equalsIgnoreCase("Query") && tables.contains(action)){
				System.out.println("Selected " + action.toUpperCase());
				System.out.println("Enter a number (1-100) to randomly select rows from " + action);
				String input = in.nextLine();
				exit = queryTable(con, action, input);
			} else if (action.equalsIgnoreCase("QUERY")){
				System.out.println("Enter a valid PSQL query");
				String input = in.nextLine();
				exit = makeQuery(con, input);
			} else {
				System.out.println("An error occured, please try again");
				exit = false;
			}
			
		} while (!exit);
		
	
	}
	
	public static boolean makeQuery(Connection con, String input) throws SQLException{
		try {
			
			Statement stmt2 = con.createStatement();
			ResultSet rs2 = stmt2.executeQuery(input);
		
			if (!rs2.next()) {
					  System.out.println("no rows fetched");
					  return false;
			}
			
			ResultSetMetaData rsmd = rs2.getMetaData();
			
			int columns = rsmd.getColumnCount();
			
			while(rs2.next()){
				String rowString = "";
				for (int y = 1; y <= columns; y++){
					rowString += rs2.getString(y) + "\t";
				}
				System.out.println(rowString);
			}
			
			stmt2.close();
			rs2.close();
		
		} catch (SQLException e){
			e.printStackTrace();
		}
		
		return false;
	}
	
	/**
	 * User provides a table they wuold like to query then provides number of rows they would like to return
	 * Knuth algo gets random row numbers to return and prints the resulting rows
	 */
	public static boolean queryTable(Connection con, String table, String input) throws SQLException{
		
		try {
			
			Integer totalCount = 0;
			Statement stmt2 = con.createStatement();
			ResultSet rs2 = stmt2.executeQuery("SELECT count(*) as TOTAL FROM " + table);
			while (rs2.next()){
					System.out.println("Count: " + rs2.getInt("total"));
					totalCount = rs2.getInt("total");
			}
		
			//use knuth algo to randomly select rows
			int getRows = Integer.parseInt(input);
			knuthAlgo(totalCount, getRows);
	
			for (int i = 0; i < rowsToVisit.size(); i++) {
				
				Statement st = con.createStatement();
				ResultSet rs = st.executeQuery("SELECT * FROM (select *, row_number() over () as rownum from " + table 
							+")temp where temp.rownum =" + rowsToVisit.get(i));
				
				ResultSetMetaData rsmd = rs.getMetaData();
				int columns = rsmd.getColumnCount();
				
				//print column headers once
				if (i == 0){
					System.out.println("**************************SAMPLING RESULTS*************************");
					String columnString = "";
					for (int x = 1; x  <= columns; x++){
							columnString += rsmd.getColumnName(x).toUpperCase() + "\t";
					}					
					System.out.println(columnString);
				}
				
				//print selected rows
				while(rs.next()){
					String rowString = "";
					for (int y = 1; y <= columns; y++){
						rowString += rs.getString(y) + "\t";
					}
					System.out.println(rowString);
				}
				
				st.close();
				rs.close();
			}
			
		} catch (SQLException e){
			e.printStackTrace();
		}
		//clear array
		rowsToVisit.clear();
		return false;
		
	}
	
	
	/**
	 * Given the total row count of the table and number of desired rows, use the knuth
	 * algorighm to create an array of rows to visit
	 * 
	 * @param totalCount
	 * @param numberOfRowsDesired
	 */
	public static void knuthAlgo(int totalCount, int numberOfRowsDesired){
		
		int totalSelected = 0;
		int totalVisited = 0;

		Boolean finished = false;
		
		while (!finished){
			Double randomNumber = Math.random();
			if (((totalCount - totalVisited)*randomNumber) >= (numberOfRowsDesired - totalSelected)){
				totalVisited += 1;
				finished = false;
			} else {
				rowsToVisit.add(totalVisited + 1);
				totalSelected += 1;
				totalVisited +=1 ;
				if (totalSelected < numberOfRowsDesired){
					finished = false;
				} else {
					finished = true;
				}
			}
		}
		
		System.out.println("Selecting the following rows " + rowsToVisit);
	
	}
	
}
