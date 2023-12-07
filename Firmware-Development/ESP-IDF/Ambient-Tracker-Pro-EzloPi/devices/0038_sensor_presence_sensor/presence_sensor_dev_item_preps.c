#include "esp_err.h"
#include "ezlopi_devices_list.h"
#include "ezlopi_cloud_constants.h"
#include "0038_sensor_presence_sensor.h"

static int __prepare_presence_sensor_distance_device_item(uint32_t parent_id, s_device_args_t *device_args);
static int __prepare_presence_sensor_velocity_device_item(uint32_t parent_id, s_device_args_t *device_args);
static int __prepare_presence_sensor_vertical_angle_device_item(uint32_t parent_id, s_device_args_t *device_args);
static int __prepare_presence_sensor_horizontal_angle_device_item(uint32_t parent_id, s_device_args_t *device_args);
static int __prepare_presence_sensor_presence_and_motion_direction_device_item(uint32_t *parent_id, s_device_args_t *device_args);

int presence_sensor_dev_item_prep(s_ezlopi_prep_arg_t *prep_args, s_device_args_t *device_args)
{
    int ret = 0;
    uint32_t parent_id = 0;

    __prepare_presence_sensor_presence_and_motion_direction_device_item(&parent_id, device_args); // parent device
    __prepare_presence_sensor_horizontal_angle_device_item(parent_id, device_args);
    __prepare_presence_sensor_vertical_angle_device_item(parent_id, device_args);
    __prepare_presence_sensor_velocity_device_item(parent_id, device_args);
    __prepare_presence_sensor_distance_device_item(parent_id, device_args);

    return ret;
}

static int __prepare_presence_sensor_presence_and_motion_direction_device_item(uint32_t *parent_id, s_device_args_t *device_args)
{
    int ret = 0;

    l_ezlopi_device_t *presence_and_motion_direction_device = ezlopi_device_add_device();
    if (presence_and_motion_direction_device)
    {
        *parent_id = ezlopi_cloud_generate_gateway_id();
        static const char *device_name = "Presence Sensor";
        ASSIGN_DEVICE_NAME_V2(presence_and_motion_direction_device, device_name);

        presence_and_motion_direction_device->cloud_properties.category = category_generic_sensor;
        presence_and_motion_direction_device->cloud_properties.subcategory = subcategory_presence;
        presence_and_motion_direction_device->cloud_properties.device_id = *parent_id;
        presence_and_motion_direction_device->cloud_properties.parent_device_id = 0;
        presence_and_motion_direction_device->cloud_properties.gateway_id = ezlopi_cloud_get_gateway_id();
        presence_and_motion_direction_device->cloud_properties.device_type = dev_type_sensor;

        l_ezlopi_item_t *presence_item = ezlopi_device_add_item_to_device(presence_and_motion_direction_device, presence_sensor_v3);
        if (presence_item)
        {
            presence_item->cloud_properties.has_getter = true;
            presence_item->cloud_properties.has_setter = false;
            presence_item->cloud_properties.show = true;
            presence_item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
            presence_item->cloud_properties.item_name = ezlopi_item_name_presence;
            presence_item->cloud_properties.value_type = value_type_token;
            presence_item->cloud_properties.scale = NULL;
            presence_item->interface_type = EZLOPI_DEVICE_INTERFACE_SPI_MASTER;

            presence_item->interface.spi_master.enable = true;
            presence_item->interface.spi_master.cs = 10;
            presence_item->interface.spi_master.sck = 8;
            presence_item->interface.spi_master.mosi = 7;
            presence_item->interface.spi_master.miso = 6;
            presence_item->user_arg = device_args;
        }

        l_ezlopi_item_t *motion_direction_item = ezlopi_device_add_item_to_device(presence_and_motion_direction_device, presence_sensor_v3);
        if (motion_direction_item)
        {
            motion_direction_item->cloud_properties.has_getter = true;
            motion_direction_item->cloud_properties.has_setter = false;
            motion_direction_item->cloud_properties.show = true;
            motion_direction_item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
            motion_direction_item->cloud_properties.item_name = ezlopi_item_name_motion_direction;
            motion_direction_item->cloud_properties.value_type = value_type_token;
            motion_direction_item->cloud_properties.scale = NULL;
            motion_direction_item->interface_type = EZLOPI_DEVICE_INTERFACE_SPI_MASTER;

            motion_direction_item->interface.spi_master.enable = false;
            motion_direction_item->interface.spi_master.cs = 10;
            motion_direction_item->interface.spi_master.sck = 8;
            motion_direction_item->interface.spi_master.mosi = 7;
            motion_direction_item->interface.spi_master.miso = 6;
            motion_direction_item->user_arg = device_args;
        }

        if (!presence_item && !motion_direction_item)
        {
            ezlopi_device_free_device(presence_and_motion_direction_device);
        }
        else
        {
            ret = 1;
        }
    }

    return ret;
}

static int __prepare_presence_sensor_horizontal_angle_device_item(uint32_t parent_id, s_device_args_t *device_args)
{
    int ret = 0;

    l_ezlopi_device_t *horizontal_angle_device = ezlopi_device_add_device();
    if (horizontal_angle_device)
    {
        char *device_name = "Object Angle : Horizontal";
        ASSIGN_DEVICE_NAME_V2(horizontal_angle_device, device_name);

        horizontal_angle_device->cloud_properties.category = category_generic_sensor;
        horizontal_angle_device->cloud_properties.subcategory = subcategory_presence;
        horizontal_angle_device->cloud_properties.device_id = ezlopi_cloud_generate_device_id();
        horizontal_angle_device->cloud_properties.parent_device_id = parent_id;
        horizontal_angle_device->cloud_properties.gateway_id = ezlopi_cloud_get_gateway_id();
        horizontal_angle_device->cloud_properties.device_type = dev_type_sensor;

        l_ezlopi_item_t *horizontal_angle_item = ezlopi_device_add_item_to_device(horizontal_angle_device, presence_sensor_v3);
        if (horizontal_angle_item)
        {
            horizontal_angle_item->cloud_properties.has_getter = true;
            horizontal_angle_item->cloud_properties.has_setter = false;
            horizontal_angle_item->cloud_properties.show = true;
            horizontal_angle_item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
            horizontal_angle_item->cloud_properties.item_name = ezlopi_item_name_angle_position;
            horizontal_angle_item->cloud_properties.value_type = value_type_angle;
            horizontal_angle_item->cloud_properties.scale = scales_north_pole_degress;
            horizontal_angle_item->interface_type = EZLOPI_DEVICE_INTERFACE_SPI_MASTER;

            horizontal_angle_item->interface.spi_master.enable = false;
            horizontal_angle_item->interface.spi_master.cs = 10;
            horizontal_angle_item->interface.spi_master.sck = 8;
            horizontal_angle_item->interface.spi_master.mosi = 7;
            horizontal_angle_item->interface.spi_master.miso = 6;
            horizontal_angle_item->user_arg = device_args;
        }
        else
        {
            ezlopi_device_free_device(horizontal_angle_device);
            ret = 1;
        }
    }
    else
    {
        ret = 1;
    }

    return ret;
}

static int __prepare_presence_sensor_vertical_angle_device_item(uint32_t parent_id, s_device_args_t *device_args)
{
    int ret = 0;

    l_ezlopi_device_t *vertical_angle_device = ezlopi_device_add_device();
    if (vertical_angle_device)
    {
        char *device_name = "Object Angle : Vertical";
        ASSIGN_DEVICE_NAME_V2(vertical_angle_device, device_name);

        vertical_angle_device->cloud_properties.category = category_generic_sensor;
        vertical_angle_device->cloud_properties.subcategory = subcategory_presence;
        vertical_angle_device->cloud_properties.device_id = ezlopi_cloud_generate_device_id();
        vertical_angle_device->cloud_properties.parent_device_id = parent_id;
        vertical_angle_device->cloud_properties.gateway_id = ezlopi_cloud_get_gateway_id();
        vertical_angle_device->cloud_properties.device_type = dev_type_sensor;

        l_ezlopi_item_t *vertical_angle_item = ezlopi_device_add_item_to_device(vertical_angle_device, presence_sensor_v3);
        if (vertical_angle_item)
        {
            vertical_angle_item->cloud_properties.has_getter = true;
            vertical_angle_item->cloud_properties.has_setter = false;
            vertical_angle_item->cloud_properties.show = true;
            vertical_angle_item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
            vertical_angle_item->cloud_properties.item_name = ezlopi_item_name_angle_position;
            vertical_angle_item->cloud_properties.value_type = value_type_angle;
            vertical_angle_item->cloud_properties.scale = scales_elevation_degrees;
            vertical_angle_item->interface_type = EZLOPI_DEVICE_INTERFACE_SPI_MASTER;

            vertical_angle_item->interface.spi_master.enable = false;
            vertical_angle_item->interface.spi_master.cs = 10;
            vertical_angle_item->interface.spi_master.sck = 8;
            vertical_angle_item->interface.spi_master.mosi = 7;
            vertical_angle_item->interface.spi_master.miso = 6;
            vertical_angle_item->user_arg = device_args;
        }
        else
        {
            ezlopi_device_free_device(vertical_angle_device);
            ret = 1;
        }
    }
    else
    {
        ret = 1;
    }

    return ret;
}

static int __prepare_presence_sensor_velocity_device_item(uint32_t parent_id, s_device_args_t *device_args)
{
    int ret = 0;

    l_ezlopi_device_t *velocity_device = ezlopi_device_add_device();
    if (velocity_device)
    {
        char *device_name = "Object Velocity";
        ASSIGN_DEVICE_NAME_V2(velocity_device, device_name);

        velocity_device->cloud_properties.category = category_generic_sensor;
        velocity_device->cloud_properties.subcategory = subcategory_presence;
        velocity_device->cloud_properties.device_id = ezlopi_cloud_generate_device_id();
        velocity_device->cloud_properties.parent_device_id = parent_id;
        velocity_device->cloud_properties.gateway_id = ezlopi_cloud_get_gateway_id();
        velocity_device->cloud_properties.device_type = dev_type_sensor;

        l_ezlopi_item_t *velocity_item = ezlopi_device_add_item_to_device(velocity_device, presence_sensor_v3);
        if (velocity_item)
        {
            velocity_item->cloud_properties.has_getter = true;
            velocity_item->cloud_properties.has_setter = false;
            velocity_item->cloud_properties.show = true;
            velocity_item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
            velocity_item->cloud_properties.item_name = ezlopi_item_name_velocity;
            velocity_item->cloud_properties.value_type = value_type_velocity;
            velocity_item->cloud_properties.scale = scales_meter_per_second;
            velocity_item->interface_type = EZLOPI_DEVICE_INTERFACE_SPI_MASTER;

            velocity_item->interface.spi_master.enable = false;
            velocity_item->interface.spi_master.cs = 10;
            velocity_item->interface.spi_master.sck = 8;
            velocity_item->interface.spi_master.mosi = 7;
            velocity_item->interface.spi_master.miso = 6;
            velocity_item->user_arg = device_args;
        }
        else
        {
            ezlopi_device_free_device(velocity_device);
            ret = 1;
        }
    }
    else
    {
        ret = 1;
    }

    return ret;
}

static int __prepare_presence_sensor_distance_device_item(uint32_t parent_id, s_device_args_t *device_args)
{
    int ret = 0;

    l_ezlopi_device_t *distance_device = ezlopi_device_add_device();
    if (distance_device)
    {
        char *device_name = "Object Distance";
        ASSIGN_DEVICE_NAME_V2(distance_device, device_name);

        distance_device->cloud_properties.category = category_generic_sensor;
        distance_device->cloud_properties.subcategory = subcategory_presence;
        distance_device->cloud_properties.device_id = ezlopi_cloud_generate_device_id();
        distance_device->cloud_properties.parent_device_id = parent_id;
        distance_device->cloud_properties.gateway_id = ezlopi_cloud_get_gateway_id();
        distance_device->cloud_properties.device_type = dev_type_sensor;

        l_ezlopi_item_t *distance_item = ezlopi_device_add_item_to_device(distance_device, presence_sensor_v3);
        if (distance_item)
        {
            distance_item->cloud_properties.has_getter = true;
            distance_item->cloud_properties.has_setter = false;
            distance_item->cloud_properties.show = true;
            distance_item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
            distance_item->cloud_properties.item_name = ezlopi_item_name_distance;
            distance_item->cloud_properties.value_type = value_type_length;
            distance_item->cloud_properties.scale = scales_meter;
            distance_item->interface_type = EZLOPI_DEVICE_INTERFACE_SPI_MASTER;

            distance_item->interface.spi_master.enable = false;
            distance_item->interface.spi_master.cs = 10;
            distance_item->interface.spi_master.sck = 8;
            distance_item->interface.spi_master.mosi = 7;
            distance_item->interface.spi_master.miso = 6;
            distance_item->user_arg = device_args;
        }
        else
        {
            ezlopi_device_free_device(distance_device);
            ret = 1;
        }
    }
    else
    {
        ret = 1;
    }

    return ret;
}
