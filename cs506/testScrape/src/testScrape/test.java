package testScrape;

import org.jsoup.Jsoup;
import org.jsoup.helper.Validate;
import org.jsoup.nodes.Document;
import org.jsoup.nodes.Element;
import org.jsoup.select.Elements;
import java.sql.*;
import java.io.IOException;

public class test {

	public static void main (String[] args) throws IOException{
		
		System.out.print("Crawling....\n");
		
		
		Document doc = Jsoup.connect("http://games.espn.go.com/ffl/leaders?").get();
		Document doc1 = Jsoup.connect("http://games.espn.go.com/ffl/leaders?&startIndex=50").get();
		Document doc2 = Jsoup.connect("http://games.espn.go.com/ffl/leaders?&startIndex=100").get();
		Document doc3 = Jsoup.connect("http://games.espn.go.com/ffl/leaders?&startIndex=150").get();
		Document doc4 = Jsoup.connect("http://games.espn.go.com/ffl/leaders?&startIndex=200").get();
		
		Elements players = doc.select("td.playertablePlayerName");
		Elements players1 = doc1.select("td.playertablePlayerName");
		Elements players2 = doc2.select("td.playertablePlayerName");
		Elements players3 = doc3.select("td.playertablePlayerName");
		
//		System.out.print("*************Top 50******************\n");
//		for (int i = 1; i < players.size(); i++){
//			System.out.print(players.get(i).text() + "\n");
//		}
//		
//		System.out.print("*************50-100******************\n");
//		for (int i = 1; i < players1.size(); i++){
//			System.out.print(players1.get(i).text() + "\n");
//		}
//		
//		System.out.print("*************101-150******************\n");
//		for (int i = 1; i < players2.size(); i++){
//			System.out.print(players2.get(i).text() + "\n");
//		}
//		System.out.print("*************151-200******************\n");
//		for (int i = 1; i < players3.size(); i++){
//			System.out.print(players3.get(i).text() + "\n");
//		}
//		
//		System.out.print("Done crawling");
	}
}
