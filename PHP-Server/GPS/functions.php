<?php

include_once('connection.php');
include_once('classes.php');
require_once('Encoding.php'); 
use \ForceUTF8\Encoding;  // It's namespaced now.

error_reporting(-1);
ini_set('display_errors', 'On');

function getUsers($con, $nombre = "") {
	$string_consulta = "SELECT * FROM users WHERE nombre LIKE %'$nombre'%;";

	$q = mysqli_query($con, $string_consulta);

	if($row = $q->fetch_assoc()){
		$user = new User((int)$row['id_user'], utf8_encode($row['nombre']), utf8_encode($row['apellido']));

		if($row['last_latitude'] && $row['last_longitude']) {
			$user->setLocation($row['last_latitude'], $row['last_longitude']);
		}
		
		$array_users = array();

		if(!empty($user)){
			array_push($array_users, $user);
		} else {
			return "";
		}

		if (!is_null($array_users) && !empty($array_users)) {
			return $array_users;	
		} else {
			return "";
		}
	} else {
		return "";
	}
}

function getUserByName($con, $nombre = "") {
	$string_consulta = "SELECT * FROM users WHERE nombre = '$nombre';";

	$q = mysqli_query($con, $string_consulta);

	if($row = $q->fetch_assoc()){
		$user = new User((int)$row['id_user'], utf8_encode($row['nombre']), utf8_encode($row['apellido']));

		if($row['last_latitude'] && $row['last_longitude']) {
			$user->setLocation($row['last_latitude'], $row['last_longitude']);
		}

		if(!empty($user)){
			return $user;
		} else {
			return "";
		}
	} else {
		return "";
	}
}

function getUserById($con, $id_user) {
	$string_consulta = "SELECT * FROM users WHERE id_user = '$id_user';";

	$q = mysqli_query($con, $string_consulta);

	if($row = $q->fetch_assoc()){
		$user = new User((int)$row['id_user'], utf8_encode($row['nombre']), utf8_encode($row['apellido']));

		if($row['last_latitude'] && $row['last_longitude']) {
			$user->setLocation($row['last_latitude'], $row['last_longitude']);
		}

		if(!empty($user)){
			return $user;
		} else {
			return null;
		}
	} else {
		return null;
	}
}

function updateUsersLocation($con, $id_user, $lat, $lng) {
	//first we need to check if the user is valid
	$user = getUserById($con, $id_user);

	//also we check if the distance is valid
	if(!is_null($user) && checkLocation($con, $id_user, $lat, $lng)){
		//we update the user's last position
		$update_users_location = "INSERT INTO user_locations (latitude, longitude, user_id) VALUES ($lat, $lng, $id_user);";
		$q = mysqli_query($con, $update_users_location);

		if($q) {
			return true;
		} else {
			return false;
		}
	} else {
		return false;
	}
}

function checkLocation($con, $id_user, $lat, $lng){
	//we check that the given location's distance is greater than $max_distance IN METERS
	$max_distance = 10;

	$distance_query = "SELECT
							(((acos(sin(($lat*pi()/180)) * sin((user_locations.latitude*pi()/180))+cos(($lat*pi()/180)) * cos((user_locations.latitude*pi()/180)) * cos((($lng- user_locations.longitude)*pi()/180))))*180/pi())*60*1.1515*1.609344) AS distance
						FROM
							user_locations, users
						WHERE
							user_locations.user_id = users.id_user AND
							users.id_user=$id_user
						ORDER BY user_locations.date DESC
						LIMIT 1;";
	$q = mysqli_query($con, $distance_query);

	if($row = $q->fetch_assoc()) {

		if(!is_null($row['distance'])) {
			return round($row['distance'] * 1000, 0) >= $max_distance;
		} else {
			//the first one
			return true;
		}
		
	} else {
		//by default it's biggest
		return true;
	}
}

function getUsersCurrentLocation($con, $id_user, $lat, $lng) {
	$distance_query = "SELECT
							user_locations.latitude,
							user_locations.longitude
						FROM
							user_locations, users
						WHERE
							user_locations.user_id = users.id_user AND
							users.id_user=$id_user
						ORDER BY user_locations.date DESC
						LIMIT 1;";
	$q = mysqli_query($con, $distance_query);

	if($row = $q->fetch_assoc()) {
		return $row['latitude'] . ",". $row['longitude'];
	} else {
		return null;
	}
}

function getNearbyPlace($con, $id_user, $lat, $lng, $radius) {
	//first we need to check if the user is valid
	$user = getUserById($con, $id_user);

	if(!is_null($user)){
		//we update the user's last position
		updateUsersLocation($con, $id_user, $lat, $lng);

		//also update the last location in the user's table
		$update_users_location = "UPDATE users SET last_latitude = $lat, last_longitude = $lng WHERE id_user = $id_user";
		$q = mysqli_query($con, $update_users_location);

		if($q) {
			//succesfull...now we need to find the places

			//first we retrieve all categories that the user has subscribed to
			//then we need all establishments that have category_id_1 or category_id_2 as that category 
			//finally we need to filter it by distance
			$nearby_places = "SELECT 
									establishments.id_establishment,
									(((acos(sin(($lat*pi()/180)) * sin((establishments.latitude*pi()/180))+cos(($lat*pi()/180)) * cos((establishments.latitude*pi()/180)) * cos((($lng- establishments.longitude)*pi()/180))))*180/pi())*60*1.1515*1.609344) AS distance,
									establishments.name AS Lugar, 
									categories.id_category,
									categories.name, 
									categories.color, 
								    establishments.latitude,
								    establishments.longitude 
								FROM 
									user_categories 
									JOIN categories ON categories.id_category = user_categories.category_id 
									JOIN establishments ON establishments.category_id_1 = user_categories.category_id 
									OR establishments.category_id_2 = user_categories.category_id 
									JOIN users ON users.id_user = user_categories.user_id 
								WHERE 
								    users.id_user = $id_user
								GROUP BY 
								    establishments.name
								HAVING
									distance <= $radius
								ORDER BY 
								    distance ASC
								LIMIT 1";

			$q = mysqli_query($con, $nearby_places);

			if($row = $q->fetch_assoc()){

				$category = new Category($row["id_category"], Encoding::fixUTF8($row["name"]), $row["color"]);
				
				$establishment = new Establishment($row["id_establishment"], $row['distance'], 
					Encoding::fixUTF8($row["Lugar"]), $row["latitude"], $row["longitude"], $category);

		    	if(!empty($establishment)){
		    		return $establishment;
		    	} else {
		    		return null;
		    	}
			} else {
				return null;
			}
		} else {
			return null;
		}
	} else {
		return null;
	}
}

function getNearbyPlaces($con, $id_user, $lat, $lng, $radius) {
	//first we need to check if the user is valid
	$user = getUserById($con, $id_user);

	if(!is_null($user)){
		//we update the user's last position
		updateUsersLocation($con, $id_user, $lat, $lng);

		//also update the last location in the user's table
		$update_users_location = "UPDATE users SET last_latitude = $lat, last_longitude = $lng WHERE id_user = $id_user";
		$q = mysqli_query($con, $update_users_location);

		if($q) {
			//succesfull...now we need to find the places

			//first we retrieve all categories that the user has subscribed to
			//then we need all establishments that have category_id_1 or category_id_2 as that category 
			//finally we need to filter it by distance
			$nearby_places = "SELECT 
									establishments.id_establishment,
									(((acos(sin(($lat*pi()/180)) * sin((establishments.latitude*pi()/180))+cos(($lat*pi()/180)) * cos((establishments.latitude*pi()/180)) * cos((($lng- establishments.longitude)*pi()/180))))*180/pi())*60*1.1515*1.609344) AS distance,
									establishments.name AS Lugar, 
									categories.id_category,
									categories.name, 
									categories.color, 
								    establishments.latitude,
								    establishments.longitude 
								FROM 
									user_categories 
									JOIN categories ON categories.id_category = user_categories.category_id 
									JOIN establishments ON establishments.category_id_1 = user_categories.category_id 
									OR establishments.category_id_2 = user_categories.category_id 
									JOIN users ON users.id_user = user_categories.user_id 
								WHERE 
								    users.id_user = $id_user
								GROUP BY 
								    establishments.name
								HAVING
									distance <= $radius
								ORDER BY 
								    distance ASC";

			$q = mysqli_query($con, $nearby_places);

			$array_places = array();

			if (mysqli_num_rows($q) > 0) {

			    while($row = mysqli_fetch_assoc($q)) {
			    	$category = new Category($row["id_category"], Encoding::fixUTF8($row["name"]), $row["color"]);

					$establishment = new Establishment($row["id_establishment"], $row['distance'], 
						Encoding::fixUTF8($row["Lugar"]), $row["latitude"], $row["longitude"], $category);

			    	if(!empty($establishment)){
			    		array_push($array_places, $establishment);
			    	}
			    }
			}

			if (!is_null($array_places) && !empty($array_places)) {
				return $array_places;	
			} else {
				return null;
			}
		} else {
			return null;
		}
	} else {
		return null;
	}
}

function getNumberOfVibrations($distance) {
	//115, 75 y 20
	if($distance <= 115 && $distance >= 75) {
		return 1;//1
	} else if($distance < 75 && $distance >= 20) {
		return 3;//3
	} else if($distance < 20){
		return 5;//5
	}else{
		return 0;	
	}
}

?>