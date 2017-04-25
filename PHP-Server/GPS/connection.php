<?php

	$link =  mysqli_connect('localhost', 'root', '');
	
	if (!$link) {
		die('No pudo conectarse: ' . mysql_error());
	}

	//echo 'Conectado  satisfactoriamente';

	$db_selected = mysqli_select_db($link, 'geovat');

	if (!$db_selected) {
		die ('No se puede conectar a la BD: ' . mysql_error());
	}

?>