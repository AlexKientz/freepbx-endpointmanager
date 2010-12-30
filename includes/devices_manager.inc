<?PHP
/**
 * Endpoint Manager Devices Manager File
 *
 * BLEEEECKKKKKK, There I just puked all over this file. That's basically what it looks like in terms of code.
 * It's a hacked-together POS written by me (Andrew) and I really need to fix it ASAP!! ah!
 *
 * @author Andrew Nagy
 * @license MPL / GPLv2 / LGPL
 * @package Provisioner
 *
 */

//Set opened variables
$message = NULL;
$error_message = NULL;

print_r($_REQUEST);

$no_add = FALSE;
$final = NULL;
$button = NULL;
$searched = NULL;
$edit = NULL;
$mode = NULL;

$family_list =& $endpoint->endpoint_data->all_products();
$full_device_list =& $endpoint->endpoint_data->all_devices();

if((!$family_list) && (!$full_device_list)) {
    $message = _("Welcome to Endpoint Manager")."!<br />"._("You have no products (Modules) installed, click")." <a href=\"config.php?display=epm_config&type=$type\">"._("here")."</a> "._("to install some");
    $no_add = TRUE;
} elseif(!$family_list) {
    $message = "Thanks for upgrading to version 2.0! Please head on over to <a href=\"config.php?display=epm_config&type=$type\">\"Brand Configurations/Setup\"</a> to setup and install phone configurations";
    $no_add = TRUE;
} elseif($endpoint->global_cfg['srvip'] == "") {
    $message = "Your Global Variables are not set! Please head on over to <a href=\"config.php?type=$type&display=epm_advanced\">\"Advanced Settings\"</a> to setup your configuration";
    $no_add = TRUE;
}

if((isset($_REQUEST['sub_type'])) AND (!$no_add)) {
    $sub_type = $_REQUEST['sub_type'];
    if(isset($_REQUEST['sub_type_sub'])) {
        $sub_type_sub = $_REQUEST['sub_type_sub'];
    } else {
        $sub_type_sub = "";
    }
} else {
    $sub_type = "";
}

switch ($sub_type) {
    //Edit Mode
    case "edit":
        switch ($sub_type_sub) {
            case "add_line_x":
                $_REQUEST['id'] = $_REQUEST['edit_id'];
                if($endpoint->linesAvailable(NULL,$_REQUEST['id'])) {
                    if($endpoint->endpoint_data->all_unused_registrations()) {
                        $sql = 'SELECT * FROM `endpointman_line_list` WHERE `mac_id` = '. $_REQUEST['id'];
                        $lines_list =& $endpoint->db->getAll($sql,array(),DB_FETCHMODE_ASSOC);

                        foreach($lines_list as $row) {
                            $sql = "SELECT description FROM devices WHERE id = ".$_REQUEST['ext_list_'.$row['luid']];
                            $name=$endpoint->db->getOne($sql);

                            $sql = "UPDATE endpointman_line_list SET line = '".$_REQUEST['line_list_'.$row['luid']]."', ext = ".$_REQUEST['ext_list_'.$row['luid']].", description = '".$name."' WHERE luid =  ". $row['luid'];
                            $endpoint->db->query($sql);
                        }

                        $reg = array_values($endpoint->display_registration_list());
                        $lines = array_values($endpoint->linesAvailable(NULL,$_REQUEST['id']));

                        $sql = "SELECT description FROM devices WHERE id = ".$reg[0]['value'];
                        $name=$endpoint->db->getOne($sql);

                        $sql = "INSERT INTO `endpointman_line_list` (`mac_id`, `ext`, `line`, `description`) VALUES ('".$_REQUEST['id']."', '".$reg[0]['value']."', '".$lines[0]['value']."', '".$name."')";
                        $endpoint->db->query($sql);
                        $message = "Configuration Files will not be Generated until you click Save!";
                    } else {
                        $message = _("No Devices/Extensions Left to Add")."!";
                    }
                } else {
                    $message = _("No Lines Left to Add")."!";
                }
                break;
            case "button_edit":
                if(empty($_REQUEST['edit_id'])) {
                    $message = _("No Device Selected to Edit!")."!";
                } else {
                    $template_editor = TRUE;
                    $sql = "UPDATE  endpointman_mac_list SET  model =  '".$_REQUEST['model_list']."' WHERE  id =".$_REQUEST['edit_id'];
                    $endpoint->db->query($sql);
                    if ($_REQUEST['template_list'] == 0) {
                        $endpoint->edit_template_display($_REQUEST['edit_id'],1);
                    } else {
                        $endpoint->edit_template_display($_REQUEST['template_list'],0);
                    }
                }
                break;
            case "button_save":
                $sql = 'SELECT * FROM endpointman_line_list WHERE mac_id = '. $_REQUEST['edit_id'];

                $lines_list =& $endpoint->db->getAll($sql,array(),DB_FETCHMODE_ASSOC);

                foreach($lines_list as $row) {
                    $sql = "SELECT description FROM devices WHERE id = ".$_REQUEST['ext_list_'.$row['luid']];
                    $name=$endpoint->db->getOne($sql);

                    $sql = "UPDATE endpointman_line_list SET line = '".$_REQUEST['line_list_'.$row['luid']]."', ext = ".$_REQUEST['ext_list_'.$row['luid']].", description = '".$name."' WHERE luid =  ". $row['luid'];
                    $endpoint->db->query($sql);
                }

                $sql = "UPDATE endpointman_mac_list SET template_id = '".$_REQUEST['template_list']."', model = '".$_REQUEST['model_list']."' WHERE id =  ". $_REQUEST['edit_id'];
                $endpoint->db->query($sql);

                $row = $endpoint->get_phone_info($_REQUEST['edit_id']);
                $endpoint->prepare_configs($row);

                $message = _("Saved")."!";
                break;
            case "delete":
                $sql = 'SELECT mac_id FROM endpointman_line_list WHERE luid = '.$_REQUEST['edit_id'] ;
                $mac_id =& $endpoint->db->getOne($sql,array(),DB_FETCHMODE_ASSOC);
                $row = $endpoint->get_phone_info($mac_id);

                $sql = "DELETE FROM endpointman_line_list WHERE luid=".$_REQUEST['edit_id'];
                $endpoint->db->query($sql);
                $_REQUEST['edit_id'] = $mac_id;
                $message = _("Deleted")."!";
                break;
        }
        $edit_row=$endpoint->get_phone_info($_REQUEST['edit_id']);
        $edit_row['id'] = $_REQUEST['edit_id'];
        $mode = "EDIT";
        break;
    case "add" :
        $mac = $endpoint->mac_check_clean($_REQUEST['mac']);
        if($mac) {
            $sql = 'SELECT description FROM devices WHERE id = '.$_REQUEST['ext_list'];
            $name =& $endpoint->db->getOne($sql);

            $endpoint->sync_model($_REQUEST['model_list']);

            $sql = 'SELECT endpointman_product_list. * , endpointman_model_list.template_data, endpointman_brand_list.directory FROM endpointman_model_list, endpointman_brand_list, endpointman_product_list WHERE endpointman_model_list.id =  \''.$_REQUEST['model_list'].'\' AND endpointman_model_list.brand = endpointman_brand_list.id AND endpointman_model_list.product_id = endpointman_product_list.id';
  
            $row =& $endpoint->db->getRow($sql,array(),DB_FETCHMODE_ASSOC);

            $cfg_data = unserialize($row['template_data']);

            $count = count($cfg_data);
            $i = 0;
            while($i < $count) {
                if(array_key_exists('variable',$cfg_data[$i])) {
                    $temping = str_replace('$','',$cfg_data[$i]['variable']);
                    if(is_array($cfg_data[$i]['default_value'])) {
                        $custom_cfg_data[$temping]['value'] = '';
                    } else {
                        $custom_cfg_data[$temping]['value'] = $cfg_data[$i]['default_value'];
                    }
                }
                $i++;
            }
            if(is_array($custom_cfg_data)) {
                $custom_cfg_data = serialize($custom_cfg_data);
            } else {
                $temp = array();
                $custom_cfg_data = serialize($temp);
            }

            $sql = "INSERT INTO `endpointman_mac_list` (`mac`, `model`, `global_custom_cfg_data`, `template_id`) VALUES ('".$mac."', '".$_REQUEST['model_list']."', '".$custom_cfg_data."', '".$_REQUEST['template_list']."')";
            $endpoint->db->query($sql);

            $sql = 'SELECT last_insert_id()';
            $ext_id =& $endpoint->db->getOne($sql);

            if(!isset($_REQUEST['line_list'])) {
                $_REQUEST['line_list'] = 1;
            }

            $sql = "INSERT INTO `endpointman_line_list` (`mac_id`, `ext`, `line`, `description`) VALUES ('".$ext_id."', '".$_REQUEST['ext_list']."', '".$_REQUEST['line_list']."', '".$name."')";
            $endpoint->db->query($sql);

            if(!isset($_REQUEST['image_x'])) {
                $mac_id = $endpoint->get_phone_info($ext_id);

                $endpoint->prepare_configs($mac_id);

                $message = _("Saved")."!";
            } else {
                $_REQUEST['edit_id'] = $ext_id;
                $edit_row=$endpoint->get_phone_info($ext_id);
                $edit_row['id'] = $_REQUEST['edit_id'];
                $mode = "EDIT";
                $_REQUEST['id'] = $_REQUEST['edit_id'];
                if($endpoint->linesAvailable(NULL,$_REQUEST['id'])) {
                    if($endpoint->endpoint_data->all_unused_registrations()) {
                        $sql = 'SELECT * FROM `endpointman_line_list` WHERE `mac_id` = '. $_REQUEST['id'];
                        $lines_list =& $endpoint->db->getAll($sql,array(),DB_FETCHMODE_ASSOC);

                        foreach($lines_list as $row) {
                            $sql = "SELECT description FROM devices WHERE id = ".$_REQUEST['ext_list'];
                            $name=$endpoint->db->getOne($sql);

                            $sql = "UPDATE endpointman_line_list SET line = '".$_REQUEST['line_list']."', ext = ".$_REQUEST['ext_list'].", description = '".$name."' WHERE luid =  ". $row['luid'];
                            $endpoint->db->query($sql);
                        }

                        $reg = array_values($endpoint->display_registration_list());
                        $lines = array_values($endpoint->linesAvailable(NULL,$_REQUEST['id']));

                        $sql = "SELECT description FROM devices WHERE id = ".$reg[0]['value'];
                        $name=$endpoint->db->getOne($sql);

                        $sql = "INSERT INTO `endpointman_line_list` (`mac_id`, `ext`, `line`, `description`) VALUES ('".$_REQUEST['id']."', '".$reg[0]['value']."', '".$lines[0]['value']."', '".$name."')";
                        $endpoint->db->query($sql);

                        $edit_row=$endpoint->get_phone_info($ext_id);
                        $edit_row['id'] = $_REQUEST['edit_id'];
                        $message = "Configuration Files will not be Generated until you click Save!";
                    } else {
                        $message = _("No Devices/Extensions Left to Add")."!";
                    }
                } else {
                    $message = _("No Lines Left to Add")."!";
                }
            }
        } else {
            $message = _("Invalid MAC Address")."!";
        }
        break;
    case "edit_template" :
        if(empty($_REQUEST['edit_id'])) {
            $message = _("No Device Selected to Edit!")."!";
        } else {
            $template_editor = TRUE;
            $sql = "UPDATE  endpointman_mac_list SET  model =  '".$_REQUEST['model_list']."' WHERE  id =".$_REQUEST['edit_id'];
            $endpoint->db->query($sql);
            if ($_REQUEST['template_list'] == 0) {
                $endpoint->edit_template_display($_REQUEST['edit_id'],1);
            } else {
                $endpoint->edit_template_display($_REQUEST['template_list'],0);
            }
        }
        break;
    case "" :
        break;
}

/*

    if(isset($_REQUEST['button_save'])) {
        $sql = 'SELECT * FROM endpointman_line_list WHERE mac_id = '. $_REQUEST['edit_id'];

        $lines_list =& $endpoint->db->getAll($sql,array(),DB_FETCHMODE_ASSOC);

        foreach($lines_list as $row) {
            $sql = "SELECT description FROM devices WHERE id = ".$_REQUEST['ext_list_'.$row['luid']];
            $name=$endpoint->db->getOne($sql);

            $sql = "UPDATE endpointman_line_list SET line = '".$_REQUEST['line_list_'.$row['luid']]."', ext = ".$_REQUEST['ext_list_'.$row['luid']].", description = '".$name."' WHERE luid =  ". $row['luid'];
            $endpoint->db->query($sql);
        }

        $sql = "UPDATE endpointman_mac_list SET template_id = '".$_REQUEST['template_list']."', model = '".$_REQUEST['model_list']."' WHERE id =  ". $_REQUEST['edit_id'];
        $endpoint->db->query($sql);

        $row = $endpoint->get_phone_info($_REQUEST['edit_id']);

        $endpoint->prepare_configs($row);

        $message = _("Saved")."!";
    }
    if(isset($_REQUEST['delete'])) {
        if($_REQUEST['type'] == "line") {
            $sql = 'SELECT mac_id FROM endpointman_line_list WHERE luid = '.$_REQUEST['edit_id'] ;
            $mac_id =& $endpoint->db->getOne($sql,array(),DB_FETCHMODE_ASSOC);
            $row = $endpoint->get_phone_info($mac_id);

            $sql = "DELETE FROM endpointman_line_list WHERE luid=".$_REQUEST['edit_id'];
            $endpoint->db->query($sql);
            $_REQUEST['edit_id'] = $mac_id;
        }
        $message = _("Deleted")."!";
    }
    $edit_row=$endpoint->get_phone_info($_REQUEST['edit_id']);
    $edit_row['id'] = $_REQUEST['edit_id'];
    $mode = "EDIT";
} elseif(isset($_REQUEST['button_edit'])) {

    if(empty($_REQUEST['edit_id'])) {
        $message = _("No Device Selected to Edit!")."!";
    } else {
        $template_editor = TRUE;
        $sql = "UPDATE  endpointman_mac_list SET  model =  '".$_REQUEST['model_list']."' WHERE  id =".$_REQUEST['edit_id'];
        $endpoint->db->query($sql);
        if ($_REQUEST['template_list'] == 0) {
            $endpoint->edit_template_display($_REQUEST['edit_id'],1);
        } else {
            $endpoint->edit_template_display($_REQUEST['template_list'],0);
        }
    }
} elseif(isset($_REQUEST['add_line_x'])) {
    if($endpoint->linesAvailable(NULL,$_REQUEST['edit_id'])) {
        if($endpoint->endpoint_data->all_unused_registrations()) {
            $sql = 'SELECT * FROM `endpointman_line_list` WHERE `mac_id` = '. $_REQUEST['edit)id'];
            $lines_list =& $endpoint->db->getAll($sql,array(),DB_FETCHMODE_ASSOC);

            foreach($lines_list as $row) {
                $sql = "SELECT description FROM devices WHERE id = ".$_REQUEST['ext_list_'.$row['luid']];
                $name=$endpoint->db->getOne($sql);

                $sql = "UPDATE endpointman_line_list SET line = '".$_REQUEST['line_list_'.$row['luid']]."', ext = ".$_REQUEST['ext_list_'.$row['luid']].", description = '".$name."' WHERE luid =  ". $row['luid'];
                $endpoint->db->query($sql);
            }

            $reg = array_values($endpoint->display_registration_list());
            $lines = array_values($endpoint->linesAvailable(NULL,$_REQUEST['edit_id']));

            $sql = "SELECT description FROM devices WHERE id = ".$reg[0]['value'];
            $name=$endpoint->db->getOne($sql);

            $sql = "INSERT INTO `endpointman_line_list` (`mac_id`, `ext`, `line`, `description`) VALUES ('".$_REQUEST['edit_id']."', '".$reg[0]['value']."', '".$lines[0]['value']."', '".$name."')";
            $endpoint->db->query($sql);

        } else {
            $message = _("No Devices Left to Add")."!";
        }
    } else {
        $message = _("No Lines Left to Add")."!";
    }
} elseif(isset($_REQUEST['button_save'])) {
    $sql = 'SELECT * FROM endpointman_line_list WHERE mac_id = '. $_REQUEST['edit_id'];

    $lines_list =& $endpoint->db->getAll($sql,array(),DB_FETCHMODE_ASSOC);

    foreach($lines_list as $row) {
        $sql = "SELECT description FROM devices WHERE id = ".$_REQUEST['ext_list_'.$row['luid']];
        $name=$endpoint->db->getOne($sql);

        $sql = "UPDATE endpointman_line_list SET line = '".$_REQUEST['line_list_'.$row['luid']]."', ext = ".$_REQUEST['ext_list_'.$row['luid']].", description = '".$name."' WHERE luid =  ". $row['luid'];
        $endpoint->db->query($sql);
    }

    $sql = "UPDATE endpointman_mac_list SET template_id = '".$_REQUEST['template_list']."', model = '".$_REQUEST['model_list']."' WHERE id =  ". $_REQUEST['edit_id'];
    $endpoint->db->query($sql);

    $row = $endpoint->get_phone_info($_REQUEST['edit_id']);

    $endpoint->prepare_configs($row);

    $message = _("Saved")."!";
} elseif(isset($_REQUEST['deleteready'])) {
    if($_REQUEST['type'] == "device") {
        $row = $endpoint->get_phone_info($_REQUEST['id']);

        $sql = "DELETE FROM endpointman_mac_list WHERE id=".$_REQUEST['id'];
        $endpoint->db->query($sql);
        $sql = "DELETE FROM endpointman_line_list WHERE mac_id=".$_REQUEST['id'];
        $endpoint->db->query($sql);
    } elseif($_REQUEST['type'] == "line") {
        $sql = 'SELECT mac_id FROM endpointman_line_list WHERE luid = '.$_REQUEST['id'] ;
        $mac_id =& $endpoint->db->getOne($sql,array(),DB_FETCHMODE_ASSOC);
        $row = $endpoint->get_phone_info($mac_id);

        $sql = "DELETE FROM endpointman_line_list WHERE luid=".$_REQUEST['id'];
        $endpoint->db->query($sql);
    }
    $message = _("Deleted")."!";
} elseif((isset($_REQUEST['button_add_selected_phones'])) AND (isset($_REQUEST['add']))) {
    $list_num = count($_REQUEST['add']);
    for($i=0;$i<$list_num;$i++) {
        $num = $_REQUEST['add'][$i];
        if($mac = $endpoint->mac_check_clean($_REQUEST['mac_'.$num])) {
            $sql = "SELECT * FROM endpointman_line_list WHERE ext = ".$_REQUEST['ext_list_'.$num];
            $res =& $endpoint->db->query($sql);
            $count_macs = $res->numRows();
            if(!$count_macs) {
                $sql = 'SELECT `name` FROM `users` WHERE `extension` = '.$_REQUEST['ext_list_'.$num];
                $name =& $endpoint->db->getOne($sql,array(),DB_FETCHMODE_ASSOC);


                //$sql = 'SELECT endpointman_product_list. * , endpointman_brand_list.directory FROM endpointman_model_list, endpointman_brand_list, endpointman_product_list WHERE endpointman_model_list.id =  \''.$_REQUEST['model_list_'.$num].'\' AND endpointman_model_list.brand = endpointman_brand_list.id AND endpointman_model_list.product_id = endpointman_product_list.id';
                //$row =& $endpoint->db->getAll($sql,array(),DB_FETCHMODE_ASSOC);


                $sql = "INSERT INTO `endpointman_mac_list` (`mac`, `model`, `template_id`) VALUES ('".$mac."', '".$_REQUEST['model_list_'.$num]."', '0')";
                $endpoint->db->query($sql);

                $sql = 'SELECT last_insert_id()';
                $ext_id =& $endpoint->db->getOne($sql);

                if(!isset($_REQUEST['line_list_'.$num])) {
                    $_REQUEST['line_list_'.$num] = 1;
                }

                $sql = "INSERT INTO `endpointman_line_list` (`mac_id`, `ext`, `line`, `description`) VALUES ('".$ext_id."', '".$_REQUEST['ext_list_'.$num]."', '".$_REQUEST['line_list_'.$num]."', '".$name."')";
                $endpoint->db->query($sql);

                $row_new = $endpoint->get_phone_info($ext_id);
                $endpoint->prepare_configs($row_new);

                $message = _("Saved")."!";
            } else {
                $message = _("One or more selected phones had the same extension as a phone already added, these devices were not added");
            }

        } else {
            $message = _("Invalid MAC Address");
        }
    }
} elseif((isset($_REQUEST['button_update_phones'])) AND (isset($_REQUEST['selected']))) {
    if(($_REQUEST['brand_list_selected'] > 0) AND ($_REQUEST['model_list_selected'] > 0)) {
        foreach($_REQUEST['selected'] as $key => $data) {
            $sql = "UPDATE endpointman_mac_list SET custom_cfg_data = '', custom_cfg_template = '', user_cfg_data = '', config_files_override = '', model = '".$_REQUEST['model_list_selected']."' WHERE id =  ". $_REQUEST['selected'][$key];
            $endpoint->db->query($sql);

            $phone_info = $endpoint->get_phone_info($_REQUEST['selected'][$key]);
            $endpoint->prepare_configs($phone_info);

            $message = _("Saved")."!";
        }
    } else {
        $message = _("Please select a Brand and/or Model");
    }
} elseif(isset($_REQUEST['button_delete_selected_phones'])) {
    if(isset($_REQUEST['selected'])) {
        foreach($_REQUEST['selected'] as $key => $data) {
            $sql = "DELETE FROM endpointman_mac_list WHERE id=".$_REQUEST['selected'][$key];
            $endpoint->db->query($sql);
            $sql = "DELETE FROM endpointman_line_list WHERE mac_id=".$_REQUEST['selected'][$key];
            $endpoint->db->query($sql);
        }
    } else {
        $message = _("No Phones Selected")."!";
    }
} elseif(isset($_REQUEST['button_go'])) {
    if ($_REQUEST['nmap'] == 1) {
        $temp = $endpoint->discover_new($_REQUEST['netmask']);
    } else {
        $temp = $endpoint->discover_new($_REQUEST['netmask'], FALSE);
    }
    $count = count($temp);
    $i = 0;
    while($i < $count) {
        if (!$temp[$i]['endpoint_managed']) {
            if ($temp[$i]['brand']) {
                $final[$i] = $temp[$i];
                $final[$i]['id'] = $i;
                $sqln = "SELECT * FROM endpointman_model_list WHERE enabled = 1 AND brand =".$temp[$i]['brand_id'];
                $model_list =& $endpoint->db->getAll($sqln,array(),DB_FETCHMODE_ASSOC);
                $j = 0;
                foreach($model_list as $row) {
                    $final[$i]['list'][$j] = $row;
                    $j++;
                }
            }
        }
        $i++;
    }
    if($final) {
        $final = array_values($final);
    } else {
        $final = NULL;
        $message = _("No Devices Found");
    }
    $searched = 1;
} elseif(isset($_REQUEST['button_rebuild_configs_for_all_phones'])) {
    $sql = "SELECT endpointman_mac_list.id FROM endpointman_mac_list, endpointman_brand_list, endpointman_product_list, endpointman_model_list WHERE endpointman_brand_list.id = endpointman_product_list.brand AND endpointman_product_list.id = endpointman_model_list.product_id AND endpointman_mac_list.model = endpointman_model_list.id ORDER BY endpointman_product_list.cfg_dir ASC";
    $mac_list =& $endpoint->db->getAll($sql,array(),DB_FETCHMODE_ASSOC);
    foreach($mac_list as $data) {
        $phone_info = $endpoint->get_phone_info($data['id']);
        $endpoint->prepare_configs($phone_info);
        foreach($phone_info['line'] as $line) {
            $sql = "UPDATE endpointman_line_list SET description = '".$line['description']."' WHERE luid = ".$line['luid'];
            $endpoint->db->query($sql);
        }
    }
    $message = "Rebuilt Configs & Rebooted All Phones";
} elseif(isset($_REQUEST['button_rebuild_reboot'])) {
    if($_REQUEST['product_select'] == "") {
        $message = _("Please select a product");
    } elseif($_REQUEST['template_selector'] == "") {
        $message = _("Please select a template");
    } else {
        $sql = "SELECT endpointman_mac_list.id FROM endpointman_mac_list, endpointman_brand_list, endpointman_product_list, endpointman_model_list WHERE endpointman_brand_list.id = endpointman_product_list.brand AND endpointman_product_list.id = endpointman_model_list.product_id AND endpointman_mac_list.model = endpointman_model_list.id AND endpointman_product_list.id = '".$_REQUEST['product_select']."'";
        $data =& $endpoint->db->getAll($sql,array(),DB_FETCHMODE_ASSOC);
        foreach($data as $row) {
            $sql = "UPDATE endpointman_mac_list SET template_id = '".$_REQUEST['template_selector']."' WHERE id =  ". $row['id'];
            $endpoint->db->query($sql);
            $phone_info = $endpoint->get_phone_info($row['id']);
            $endpoint->prepare_configs($phone_info);
            foreach($phone_info['line'] as $line) {
                $sql = "UPDATE endpointman_line_list SET description = '".$line['description']."' WHERE luid = ".$line['luid'];
                $endpoint->db->query($sql);
            }
        }
        $message = "Rebuilt Configs & Rebooted Selected Phones2";
    }
} elseif(isset($_REQUEST['button_rebuild_selected'])) {
    if(isset($_REQUEST['selected'])) {
        foreach($_REQUEST['selected'] as $key => $data) {
            $phone_info = $endpoint->get_phone_info($_REQUEST['selected'][$key]);
            $endpoint->prepare_configs($phone_info);
        }
        $message = "Rebuilt Configs & Rebooted Selected Phones";
    } else {
        $message = _("No Phones Selected")."!";
    }
} elseif(isset($_REQUEST['button_reboot_this_brand'])) {
    if($_REQUEST['rb_brand'] != "") {
        $sql = 'SELECT endpointman_mac_list.id FROM endpointman_mac_list , endpointman_model_list , endpointman_brand_list , endpointman_product_list WHERE endpointman_brand_list.id = endpointman_model_list.brand AND endpointman_model_list.id = endpointman_mac_list.model AND endpointman_model_list.product_id = endpointman_product_list.id AND endpointman_brand_list.id = '.$_REQUEST['rb_brand'].' ORDER BY endpointman_product_list.cfg_dir ASC';
        $data =& $endpoint->db->getAll($sql,array(),DB_FETCHMODE_ASSOC);
        foreach($data as $row) {
            if(!class_exists('ProvisionerConfig')) {
                require(PHONE_MODULES_PATH.'setup.php');
            }
            $phone_info = $endpoint->get_phone_info($row['id']);

            $class = "endpoint_" . $phone_info['directory'] . "_" . $phone_info['cfg_dir'] . '_phone';

            $provisioner_lib = new $class();

            $provisioner_lib->root_dir = PHONE_MODULES_PATH;

            $provisioner_lib->engine = 'asterisk';
            $provisioner_lib->system = 'unix';

            //have to because of versions less than php5.3
            $provisioner_lib->brand_name = $phone_info['directory'];
            $provisioner_lib->family_line = $phone_info['cfg_dir'];

            $provisioner_lib->lines[1] = array('ext' => $phone_info['line'][1]['ext']);
            $provisioner_lib->reboot();
            unset($provisioner_lib);
        }
        $message = "Rebooted all ". $phone_info['name'] . " phones";
    } else {
        $message = _("No Brand Selected for Reboot");
    }
}
 * 
 */

//Refresh the list after processing
$devices_list =& $endpoint->endpoint_data->all_devices();;

$i = 0;
$list = array();
foreach($devices_list as $devices_row) {
    $line_list =& $endpoint->endpoint_data->get_lines_from_device($devices_row['id']);
    $list[$i] = $devices_row;
    $z = 0;
    if (($devices_row['template_id'] == 0) && (isset($devices_row['global_custom_cfg_data'])) ) {
        $list[$i]['template_name'] = "Custom-".$devices_row['mac'];
    } elseif((!isset($devices_row['custom_cfg_data'])) && ($devices_row['template_id'] == 0)) {
        $list[$i]['template_name'] = "N/A";
    } else {
        $sql = "SELECT name FROM endpointman_template_list WHERE id =".$devices_row['template_id'];
        $template_name =& $endpoint->db->getOne($sql);
        $list[$i]['template_name'] = $template_name;
    }
    if (!$devices_row['enabled']) {
        $list[$i]['model'] = $devices_row['model']."<i>(Disabled)</i>";
    }
    $list[$i]['master_id'] = $i;
    foreach($line_list as $line_row) {
        $list[$i]['line'][$z]['ext'] = $line_row['ext'];
        $list[$i]['line'][$z]['line'] = $line_row['line'];
        $list[$i]['line'][$z]['description'] = $line_row['description'];
        $list[$i]['line'][$z]['luid'] = $line_row['luid'];
        $list[$i]['line'][$z]['master_id'] = $i;
        $z++;
    }
    $i++;
}

$unknown_list =& $endpoint->endpoint_data->all_unknown_devices();

foreach($unknown_list as $row) {	#Displays unknown phones in the database with edit and delete buttons
    $list[$i] = $row;

    $brand_info = $endpoint->get_brand_from_mac($row['mac']);

    $list[$i]['name'] = $brand_info['name'];
    $list[$i]['template_name'] = "N/A";
    $list[$i]['model'] = _("Unknown");
    $i++;
}

$amp_send['AMPDBUSER'] = $amp_conf['AMPDBUSER'];
$amp_send['AMPDBPASS'] = $amp_conf['AMPDBPASS'];
$amp_send['AMPDBNAME'] = $amp_conf['AMPDBNAME'];

$sql = "SELECT DISTINCT endpointman_product_list.* FROM endpointman_product_list, endpointman_model_list WHERE endpointman_product_list.id = endpointman_model_list.product_id AND endpointman_model_list.hidden = 0 AND endpointman_model_list.enabled = 1 AND endpointman_product_list.hidden != 1 AND endpointman_product_list.cfg_dir !=  ''";

$template_list =& $db->getAll($sql, array(), DB_FETCHMODE_ASSOC);
$i = 1;
$product_list = array();
$product_list[0]['value'] = 0;
$product_list[0]['text'] = "";
foreach($template_list as $row) {
        $product_list[$i]['value'] = $row['id'];
        $product_list[$i]['text'] = $row['short_name'];
        $i++;
}

//initialize a Rain TPL object
if (isset($template_editor)) {

} else {
    $endpoint->tpl->assign("list", $list);
    $endpoint->tpl->assign("error", "");
    $endpoint->tpl->assign("srvip", $_SERVER["SERVER_ADDR"]);
    $endpoint->tpl->assign("web_var", "?type=$type");
    $ma = $endpoint->models_available();
    if($ma != FALSE) {
        $endpoint->tpl->assign("models_ava", $ma);
    }
    $endpoint->tpl->assign("web_path", WEB_PATH);
    $endpoint->tpl->assign("product_list", $product_list);
    $endpoint->tpl->assign("display_ext", $endpoint->display_registration_list());
    $endpoint->tpl->assign("brand_ava", $endpoint->brands_available());
    $endpoint->tpl->assign("unmanaged", $final);
    $endpoint->tpl->assign("button", $button);
    $endpoint->tpl->assign("searched", $searched);
    $endpoint->tpl->assign("edit", $edit);
    $endpoint->tpl->assign("amp_conf_serial", base64_encode(serialize($amp_send)));
    $endpoint->tpl->assign("no_add", $no_add);
    $endpoint->tpl->assign("mode", $mode);

    if(isset($final)) {
        $endpoint->tpl->assign("cached_devices", base64_encode(serialize($final)));
    }

    if (isset($mode) && ($mode == "EDIT")) {
        $ma = $endpoint->models_available($edit_row['model_id'],$edit_row['brand_id']);
        if($ma != FALSE) {
            $endpoint->tpl->assign("mac", $edit_row['mac']);
            $endpoint->tpl->assign("name", $edit_row['name']);
            $b=0;
            foreach($edit_row['line'] as $data) {
                $edit_row['line'][$data['line']]['reg_list'] = $endpoint->display_registration_list($data['luid']);
                $edit_row['line'][$data['line']]['line_list'] = $endpoint->linesAvailable($data['luid']);
                $b++;
            }
            if($b == 1) {
                $endpoint->tpl->assign("disabled_delete_line", 1);
            }
            $endpoint->tpl->assign("line_list_edit", $edit_row['line']);

            $endpoint->tpl->assign("brand_id", $edit_row['brand_id']);
            $endpoint->tpl->assign("models_ava", $ma);
            //$endpoint->tpl->assign("display_ext", $endpoint->display_registration_list($edit_row['line'][2]['luid']));
            //$endpoint->tpl->assign("display_ext", $edit_row['line']);

            $endpoint->tpl->assign("display_templates", $endpoint->display_templates($edit_row['product_id'],$edit_row['template_id']));
            $endpoint->tpl->assign("edit_id", $edit_row['id']);
        } else {
            $message = _("You have disabled/removed all models that correspond to this brand. Please enable them in 'Brand Configurations/Setup' before trying to edit this phone");
            $endpoint->tpl->assign("mode", NULL);
        }
    }

    foreach($endpoint->error as $key => $error) {
        $error_message .= $error;
        if($endpoint->global_cfg['debug']) {
            $error_message .= " Function: [".$key."]";
        }
        $error_message .= "<br />";
    }

    if(isset($message)) {
        $endpoint->display_message_box($message,$endpoint->tpl,0);
    }

    if(isset($error_message)) {
        $endpoint->display_message_box($error_message,$endpoint->tpl,1);
    }

    //draw the template
    echo $endpoint->tpl->draw( 'devices_manager' );
}