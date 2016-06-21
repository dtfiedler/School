<?php
	//require ("../db/Conn.php"); --->LESS SECURE USE INI FILE
	require ("../db/MySQLDAO.php");
	require ("../Classes/PasswordReset.php");
	//hide db information an ini file outside of current directory
	$config = parse_ini_file("../../gordysTest.ini");
	$returnValue = array();

	if (empty($_POST["email"])){
		$returnValue["status"] = "400";
		$returnValue["message"] = "Email not found";
		echo json_encode($returnValue);
		return;
	}

	//html enttites helps protect from sql injections
	$firstName= htmlentities($_POST["email"]);
	
	//get variables from ini file
	$dbhost = trim($config["dbhost"]);
	$dbuser = trim($config["dbuser"]);
	$dbpass = trim($config["dbpass"]);
	$dbname = trim($config["dbname"]);

	//connect to database
	$dao = new MySQLDAO($dbhost, $dbuser, $dbpass, $dbname);
	$dao->openConnection();

	//check if user already exists
	$userDetails = $dao->getUserDetails($email);
	if (!empty($userDetails)){
		$returnValue["message"] = "Email not found";
		echo json_encode($returnValue);
		return;
	}

	$user_id = userDetails["user_id"];

	//generate new token
	$passwordReset = new PasswordReset();
	$passwordToken = $passwordReset->generateUniqueToken(16);

	//store new token in password Tokens DB
	$dao->storePasswordToken($user_id, $passwordToken);

	//prepare emial message with parameters
	$messageDetails = array();
	$messageDetails["message_subject"] = "Password reset requested";
	$messageDetails["to_email"] = $userDetails["email"];
	$messageDetails["from_name"] = "Gordy's Marina";
	$messageDetails["from_email"] = "gordyshelp@gmail.com";

	//load html message and insert link to reset password
	$messageBody = $passwordReset->generateMessageBody();
	$emailMessage = str_replace("{token}", $passwordToken, $messageBody);
	$messageDetails["message_body"] = $emailMessage;

	//send email message
	$passwordReset->sendEmailMessage($messageDetails);

	//return message to app
	$returnValue["email"] = $email;
	$returnValue["message"] = "We have sent you an email address to reset your password";

	echo json_encode($returnValue);


$dao->closeConnection();

?>