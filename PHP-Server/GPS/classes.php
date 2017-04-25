<?php

class Category {

	var $id_category;
	var $name;
	var $color;

	function __construct($id_category, $name, $color) {
		$this -> id_category = $id_category;
		$this -> name = $name;
		$this -> color = $color;
	}
}

class Establishment {

	var $id_establishment;
	var $distance;
	var $name;
	var $latitude;
	var $longitude;

	//the Category object
	var $category;

	function __construct($id_establishment, $distance, $name, $latitude, $longitude, $category){
		$this -> id_establishment = $id_establishment;
		$this -> distance = round($distance  * 1000, 0);
		$this -> name = $name;
		$this -> latitude = $latitude;
		$this -> longitude = $longitude;
		$this -> category = $category;
	}
}

class User {

	var $id_user;
	var $nombre;
	var $apellido;
	var $last_latitude;
	var $last_longitude;

	function __construct($id_user, $nombre, $apellido){
		$this -> id_user = $id_user;
		$this -> nombre = $nombre;
		$this -> apellido = $apellido;
	}

	function setLocation($latitude, $longitude) {
		$this -> last_latitude = $latitude;	
		$this -> last_longitude = $longitude;
	}
}

class Conexion {

	var $connection;
	var $status = array();

	function __construct($host, $user, $password, $db) {
		$this -> connection = mysqli_connect($host, $user, $password);

		if (!$this -> connection) {
			$this -> setStatus("Error al tratar de conectar. Revise las credenciales de Acceso");
			die('No pudo establecer conexión con a $h: ' . mysql_error());
		}
		$this -> setStatus("Exito al tratar de conectar");

		$database = mysqli_select_db($this -> connection, $db);

		if (!$database) {
			$this -> setStatus("Error al tratar de ingresar a la Base de Datos. Revise el nombre de la Base de Datos");
			die ('No se puede conectar a $db: ' . mysql_error());
		}
		$this -> setStatus("Exito al tratar de conectarse a la Base de Datos");

	}
	
	function __destruct(){
		if(mysqli_close($this -> connection)){
			$this -> setStatus("Exito al cerrar la conexion");
		} else {
			$this -> setStatus("Error al cerrar la conexion");
		}
	}
	
	function getConnection(){
		return $this -> connection;
	}

	function setStatus($s = ""){
		array_push($this -> status, $s);
	}

	function getStatus(){
		return $this -> status;
	}

	function printStatus(){
		$i = 1;		
		foreach ($this -> status as $key => $value) {
			echo "Status " . $i . ": " . $value . "<br />";
			$i++;
		}
	}
}

?>