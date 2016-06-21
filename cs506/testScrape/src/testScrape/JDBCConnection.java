package testScrape;
import java.sql.*;
import java.io.IOException;
import java.util.Random;

import org.jsoup.Jsoup;
import org.jsoup.nodes.Document;
import org.jsoup.select.Elements;


public class JDBCConnection {
	public static void main (String[] args) throws Exception {
		
		Document doc = Jsoup.connect("http://games.espn.go.com/ffl/leaders?").get();
		Elements players = doc.select("td.playertablePlayerName");
		
		Class.forName("com.mysql.jdbc.Driver"); 
		System.out.println("Driver successfully loaded");
		Connection con = DriverManager.getConnection("jdbc:mysql://ec2-user@ec2-52-36-51-196.us-west-2.compute.amazonaws.com/FFAdb", "root", "eric2mad");

		
		//		Statement stmt = con.createStatement();
//		try {
//	    String sql = "CREATE TABLE REGISTRATION " +
//                "(id INTEGER not NULL, " +
//                " first VARCHAR(255), " + 
//                " last VARCHAR(255), " + 
//                " age INTEGER, " + 
//	    
//	    stmt.executeUpdate(sql);
//		} catch (SQLException ex){
//			System.out.println(ex.getMessage());
//		}
		
		//insert into table
		String sql = "insert into Leagues set LeagueId=? ScoringType=?, Name=?, Year=?, ESPNLeagueId";
		PreparedStatement statement = con.prepareStatement(sql);
		
		Random randomGenerator = new Random();
		for (int i = 0; i < players.size(); i++){
			int randomInt = randomGenerator.nextInt(1000);
			statement.setInt(1, randomInt);
			statement.setInt(2, i);
			statement.setString(3, players.get(i).text().toString());
			statement.setInt(4, i);
			statement.setInt(5, i);
			statement.executeUpdate();
		}
		
		
		
		//retrieve all
//		stmt = con.createStatement();
//		sql = "Select * from REGISTRATION";
//		ResultSet result = stmt.executeQuery(sql);
//		System.out.print(result[0]);
//	    
//	    printTables(con);
		
	}
	
	public static void printTables(Connection con){

	}
}
