<?php

include_once "classes.php";
include_once "functions.php";

error_reporting(-1);
ini_set('display_errors', 'On');

if (isset($_REQUEST['id_user']) && $_REQUEST['id_user'] !== '' &&
	isset($_REQUEST['latitude']) && $_REQUEST['latitude'] !== '' &&
	isset($_REQUEST['longitude']) && $_REQUEST['longitude'] !== '') {

	$id_user = $_REQUEST['id_user'];//es el id del usuario
	$latitude = $_REQUEST['latitude'];
	$longitude = $_REQUEST['longitude'];

	//abrimos la conexión con la Base de Datos
	$con = new Conexion('localhost', 'root', '', 'geovat');
	//$con = new Conexion('localhost', 'webmaster', '', 'geovat');

	//ahora hacemos el filtro si necesitamos toda la info o simplemente lo abreviamos
	//requerir = 0  -> TODA la información
	//requerir = 1  -> información resumida

	//por default necesitamos TODA la información
	$requerir = 0;

	if(isset($_REQUEST['requerir']) && $_REQUEST['requerir'] !== '' ) {
		$requerir = $_REQUEST['requerir'];
	}

	//we select the RADIUS in KM
	$radius = 0.15;
	$place = getNearbyPlace($con -> connection, $id_user, $latitude, $longitude, $radius);

	if(!is_null($place)){
		//si el usuario existe

		$vibrations = getNumberOfVibrations($place -> distance);
		$ledColor = $place -> category -> color;

		if($requerir === '1') {
			//toda la información
			$data = array('place' => $place, 'vibrations' => $vibrations, 'color' => $ledColor, 'location' => getUsersCurrentLocation($con -> connection, $id_user, $latitude, $longitude));
			echo json_encode($data);	
		} else {
			//información resumida
			echo "v=" . $vibrations . ",c=" . $ledColor . "*";
		}
	} else {
		echo "v=" . "0" . ",c=" . "0" . "*";
		//$data = array('message' => "error: No user with id '$id_user' or no nearby places", 'status' => 500);
		//echo json_encode($data);
	}

} else {
	$data = array('message' => "Faltan parámetros", 'status' => 500);
	echo json_encode($data);
}

?>