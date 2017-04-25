<?php

include_once "classes.php";
include_once "functions.php";

if (isset($_REQUEST['nombre']) && $_REQUEST['nombre'] !== '') {

	$name = $_REQUEST['nombre'];//es el nombre de usuario (email) del dueño del celular

	//$con = new Conexion('localhost', 'root', 'root', 'test_yimbo');
	$con = new Conexion('localhost', 'root', '', 'geovat');

	$user = getUserByName($con -> connection, $name);

	if(!is_null($user)){
		//si el usuario existe
		$data = array('message' => "OK", 'status' => 200, 'users' => $user);
		echo json_encode($data);
	} else {
		$data = array('message' => "error", 'status' => 500);
		echo json_encode($data);
	}

} else {
	$data = array('message' => "Faltan parámetros", 'status' => 500);
	echo json_encode($data);
}

?>