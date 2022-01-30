<?php

if(isset($_FILES['file']['name']))
{
   if(!is_dir("../uploads"))
      mkdir("../uploads");
   $filename = $_FILES['file']['tmp_name'];
   $pgm_file = "../uploads/myfile.pgm";

   try {
   		$image = new Imagick();
         $handle = fopen($filename, "rb");
         $image->readImageFile($handle);
   		$image->writeImage($pgm_file);
   		$image->destroy();
         fclose($handle);

   } catch (ImagickException $e) {
   		unlink($filename);
   		echo("error - converting image to PGM file: ".$e->getMessage());
   		exit;
   }
   unlink($filename);
   $command = "./web_ocr ".$pgm_file." 2>&1";
   $output = shell_exec($command);
   echo($output); 
   exit;
}

echo("error: No filename provided");

?>