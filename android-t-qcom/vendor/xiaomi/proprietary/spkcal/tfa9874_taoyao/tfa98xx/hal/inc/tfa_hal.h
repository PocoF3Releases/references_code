/************************************************************************/
/* Copyright 2014-2017 NXP Semiconductors                               */
/* Copyright 2020 GOODIX                                                */
/*                                                                      */
/* GOODIX Confidential. This software is owned or controlled by GOODIX  */
/* and may only be used strictly in accordance with the applicable      */
/* license terms.  By expressly accepting such terms or by downloading, */
/* installing, activating and/or otherwise using the software, you are  */
/* agreeing that you have read, and that you agree to comply with and   */
/* are bound by, such license terms.                                    */
/* If you do not agree to be bound by the applicable license terms,     */
/* then you may not retain, install, activate or otherwise use the      */
/* software.                                                            */
/*                                                                      */
/************************************************************************/

#if !defined(TFA_HAL_H)
#define TFA_HAL_H	1

#include <errno.h>
#include <stdio.h>

/** * @defgroup TFA_HAL target interface abstraction
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/* FIXME: TFA_HAL_MAX_BUFFER_SIZE should be defined only by plugin internally! */
/**
 * @brief Defines default maixmal buffer size for communication through HAL.
 *
 * @remark It could be changed by plugin @ref tfa_hal_buffersize.
 */
#define TFA_HAL_MAX_BUFFER_SIZE     254

struct tfa_device;
struct tfa_hal_dev;  /* Structure describes underlaying interface (opaque). */
struct tfa_hal_plugin_funcs;

/* Following definitions are used by plugins and hal layer excl.*/
#if defined(TFA_HAL_PLUGIN) || defined(TFA_HAL)

/**
 * @brief Symbol used by extrnal plugins (shared libraries) to deliver plug symbol.
 *
 * @remark tfa_hal_plug has to be exported form DLL. This is important for MS Windows platform.
 *
 */
#define PLUG_SYMBOL	"tfa_hal_plug"

/**
 * @brief Value used by plugin to verify identity of plug symbol.
 *
 * @remark If this value mismatches value defined by plugin, the plugin is not loaded.
 * @remakrk Full mechanism behind this feature is not yet implemented.
 */
#define MAGIC		"ASWLWB"


/**
 * @brief Plugin inteface. This structure defines interface available to plugins.
 *
 * This interface models all actions that could be executed by plugin, independet of plugin type.
 * Plugin type is represented by @ref tfa_hal_iftype.
 * Plugin is realization of the interface.
 * However not all methods of the interface have to be implemented for plugin to be valid.
 * List of methods required for plugin type depends on @ref tfa_hal_if.
 *
 */
struct tfa_hal_plugin_funcs
{
	/** Function responsible for plug-in initialization.
	 *
	 * init() is a mandatory function for plugin. It is responsible for plugin
	 * initialization. It might use system resources like malloc() or socket(),
	 * that are needed to realize connection to target device.
	 * Plugin is also responsible for releasing all resources taken during call to @ref close.
	 *
	 *
	 * @param[in] dev   plugin instance
	 * @param[in] param string containing parameter for plugin
	 * @param[in] len   length of string with parameters (expressed in bytes)
	 *
	 * @return 0 when successful, error code otherwise @ref hal_error_codes.
	 */
	int (*init)(struct tfa_hal_dev *dev, const char *param, size_t len);

	/** Function responsible for plug-in finalization.
	 *
	 * close() is a mandatory function for plugin. It is responsible for plugin
	 * clean-up. It is called by HAL layer when connection to a target is closed.
	 * It might use system resources like free() or close() to clean up resources
	 *
	 * Function is also responsible for releasing them during call to @ref close.
	 *
	 *
	 * @param[in] dev   plugin instance
	 *
	 * @return 0 when successful, error code otherwise @ref hal_error_codes.
	 */
	int (*close)(struct tfa_hal_dev *dev);

	/** Function responsible for transfer from or to device.
	 *
	 * write_read() is a main I/O method. It used only for I2C devices transfers.
	 * It is mandatory for none TFA_HAL_IFTYPE_MSG type of plugins.
	 * It supports logic specific for I2C devices. Each operation consists of write operation
	 * or write/read operation. To trigger write only operation out_buf and out_bytes paramters
	 * should be set to 0.
	 * This method is allowed to split transmision to match transmision protocol used by plugin.
	 * However this behaviour has to be transparent for hal layer.
	 *
	 * @param[in]  dev       handle to plugin
	 * @param[in]  address   target device address (depends on plugin, container file)
	 * @param[in]  in_buf    buffer containing data to be send to device
	 * @param[in]  in_bytes  size of in_buf expressed in bytes
	 * @param[out] out_buf   buffer to be filled with data received from device (R/W I/O only, NULL otherwise)
	 * @param[in]  out_bytes size of out_buf expressed in bytes (R/W I/O only, 0 otherwise)
	 *
	 * @return 0 when successful, error code otherwise @ref hal_error_codes.
	 */
	int (*write_read)(struct tfa_hal_dev *dev, unsigned address,
				const char *in_buf, size_t in_len,
				char *out_buf, size_t out_len);

	/** Function sets new pin value.
	 *
	 * @param[in] dev   plugin instance
	 * @param[in] pin   pin number
	 * @param[in] value new pin value
	 *
	 * @return 0 when successful, error code otherwise @ref hal_error_codes.
	 */
	int (*set_pin)(struct tfa_hal_dev *dev, int pin, int value);

	/** Function gets current pin value.
	 *
	 * @param[in]  dev    plugin instance
	 * @param[in]  pin    pin number
	 * @param[out] value  pointer to variable which receives current pin value
	 *
	 * @return 0 when successful, error code otherwise @ref hal_error_codes.
	 */
	int (*get_pin)(struct tfa_hal_dev *dev, int pin, int *value);

	/** Function returns maximal buffer size for I/O transfers done by plugin.
	 *
	 * @param[in]  dev      plugin instance
	 * @param[out] buf_size pointer to variable which receives value
	 *
	 * @return 0 when successful, error code otherwise @ref hal_error_codes.
	 */
	int (*buffersize)(struct tfa_hal_dev *dev, size_t *buf_size);

	/** Function returns string identifying plugin (user view).
	 *
	 * @param[in]  dev plugin instance
	 * @param[out] buf pointer to string buffer
	 * @param[in]  len size of string buffer
	 *
	 * @return 0 when successful, error code otherwise @ref hal_error_codes.
	 */
	int (*version_str)(struct tfa_hal_dev *dev, char *buf, size_t len);

	/* This part of interface is targeted for messaging type of interface */

	/** Function starts playback of a file.
	 *
	 * @param[in] dev plugin instance
	 * @param[in] buf name of file to play
	 * @param[in] len size of buffer buf in bytes
	 *
	 * @return 0 when successful, error code otherwise @ref hal_error_codes.
	 */
	int (*startplayback)(struct tfa_hal_dev *dev, const char *buf, size_t len);

	/** Function stops playback of current stream.
	 *
	 * @param[in] dev plugin instance
	 *
	 * @return 0 when successful, error code otherwise @ref hal_error_codes.
	 */
	int (*stopplayback)(struct tfa_hal_dev *dev);

	/** Function transfers messages directed to tfadsp instance.
	 *
	 * The call is a main I/O function for tfadsp-like devices.
	 * It is mandatory for TFA_HAL_IFTYPE_MSG type of plugins.
	 * It supports logic specific for tfadsp messaging.
	 * Each operation consists of write, read operation or write/read operation.
	 * No I2C transaction should be issued using this interface.
	 *
	 * In case of write only or read only operations some arguments could be set to NULL.
	 *
	 * @param[in]  dev     handle to plugin
	 * @param[in]  address device uniqe identifier
	 * @param[in]  cmd_buf command buffer (NULL for read-only operation)
	 * @param[in]  cmd_len size of command buffer in bytes (0 for read-only operation)
	 * @param[out] res_buf response buffer (NULL for write-only operation)
	 * @param[in]  res_len response buffer size in bytes (0 for write-only operation)
	 *
	 * @return 0 when successful, error code otherwise @ref hal_error_codes.
	 *
	 */
	int (*tfadsp_execute)(struct tfa_hal_dev *dev, unsigned address,
			      const char *cmd_buf, size_t cmd_len,
			      char *res_buf, size_t res_len);

	/** Function sends a initialization message to tfadsp-like entity.
	 *
	 * Init message's goal is to send set of params to startup tfadsp-like target.
	 * This call in special conditions could mean restart (the call is done during normal operation)
	 * However this is plugin dependent behviour.
	 * This function is mandatory for TFA_HAL_IFTYPE_MSG type of plugins.
	 *
	 * @param[in]  dev     handle to plugin
	 * @param[in]  address device uniqe identifier
	 * @param[in]  buf     command buffer
	 * @param[in]  len     size of command buffer in bytes
	 *
	 * @return 0 when successful, error code otherwise @ref hal_error_codes.
	 */
	int (*tfadsp_init)(struct tfa_hal_dev *dev, unsigned address,
			   const char *buf, size_t len);

  
	/** Function responsible for plug-in initialization to get the TFA details .
	 *
	 * init_to_get_tfa_details() is a function very similar to the init() funtion.
	 * It can be called even when the TFA is not connected. 
	 * This is a light weight 'init' function. Any specific initialization to be done without affecting normal 'init' function
	 * can be done using this function. It might use system resources like malloc() or socket(),
	 * that are needed to realize connection to target device.
	 * Plugin is also responsible for releasing all resources taken during call to @ref close.
	 * If a plugin do not need this specific init function, then the "init" function itself can be used instead of new function.
	 *
	 *
	 * @param[in] dev   plugin instance
	 * @param[in] param string containing parameter for plugin
	 * @param[in] len   length of string with parameters (expressed in bytes)
	 *
	 * @return 0 when successful, error code otherwise @ref hal_error_codes.
	 */
  int (*init_to_get_tfa_details)(struct tfa_hal_dev *dev, const char *param, size_t len);
};

/**
 * @brief tfa_hal_dev is an adaptor object. It translates plugin API (@ref tfa_hal_plugin_funcs)
 *        into HAL library API.
 *
 * This object adpapt API presented in @ref TFA_HAL into @ref tfa_hal_plugin_funcs API.
 * tfa_hal_dev object also controls life-time of plug-in.
 *
 * The object contains fields available for plug-in. This includes
 * - `priv`, used as container for plug-in dependent data (managed by plug-in)
 * - `iftype`, used to declar plug-in type (@ref tfa_hal_iftype)
 * - `verbose`, used to inform plugin about verbosity/loggin level requested by user
 * - `logout`, which contains FILE pointer for logging output
 *
 * Life-time of this object is 1:1 related with plug-in life-time in HostSDK.
 * It is created, when plug-in is loaded. Maintained alive until plug-in is used,
 * then destroyed, when plug-in should be unload. This translates to calls
 * to init() method of plugin, then to close() method during finalization stage.
 *
 */
struct tfa_hal_dev {
	struct tfa_device *tfa;
	const void *plugin; /* A plugin handle */

	/* Hal plugins functions */
	struct tfa_hal_plugin_funcs funcs;

	/* These fields are accessible for plug-in */

	void *priv; /**< Plugin private data, used only by plug-in */
	unsigned verbose; /**< Verbosity level selected by user */
	FILE *logout;  /**< Field contains stream pointer for logging(verbosity) */
	unsigned iftype; /**< Field decides type of plugi-in, set by plugin */

	/*
	 * Overlay extension
	 *
	 * These fields are accessible only by overlay plug-in.
	 * All data written here by normal plug-in will be removed
	 */
	struct tfa_hal_plugin_funcs overlay_funcs;  /**< Overlay plug-in functions */
	void *overlay_priv;  /**< Overlay plug-in private data, used only by plug-in */
};

#endif /* defined(TFA_HAL_PLUGIN) || defined(TFA_HAL) */

/* Following functions and types are an public API of tfa_hal module */

/**
 * @brief Defines HAL plugin type.
 *
 * It defines type of interface provided by plugin. two most popular are
 * TFA_HAL_IFTYPE_MSG and TFA_HAL_IFTYPE_I2C.
 * They define two different type of communication. TFA_HAL_IFTYPE_I2C interface
 * supports direct operation on I2C bus. This includes DSP messages, register accdesses, etc.
 * In case of TFA_HAL_IFTYPE_MSG, there is no communication with device other than messaging.
 * No I2C operations are possible. Messages are sent through platform defined mechanism.
 *
 * @remark Content of this type might change.
 */

enum tfa_hal_iftype {
	TFA_HAL_IFTYPE_UNKNOWN=0, 	/**< no type defined */
	TFA_HAL_IFTYPE_DIRECT=1, 	/**< no bus interface */
	TFA_HAL_IFTYPE_I2C=2, 		/**< i2c hardware interface */
	TFA_HAL_IFTYPE_HID=4, 		/**< usb hid interface */
	TFA_HAL_IFTYPE_MSG=8, 		/**< message capable interface */
	TFA_HAL_IFTYPE_PLAYBACK=0x10, /**< implement audio playback */
	TFA_HAL_IFTYPE_MULTI=0x20, /**< supports new Scribo protocol */
	TFA_HAL_IFTYPE_IfTypeMaxVale=0xff /**< max enum */
};

typedef enum tfa_hal_iftype tfa_hal_iftype;

/**
 * @brief Describes plug-in for tfa_hal_load_overlay()
 *
 */
struct tfa_hal_plug_desc {
       const char *libname;  /**< Library name for dynamic libraries plugin. Must be set in this case */
       void *plug;  /**< Reserved for tfa_hal_load_overlay */
       struct tfa_hal_plugin_funcs *symbol; /**< Plugin Symbol in case of static plugins. Must be set in this case */
       const char *params; /* Plugin parameters string, ASCIZ */
       size_t params_len; /* Plugin parameters string len including ending 0 */
};

/**
 * @brief Function ties plugin symbol with tfa_hal_device.
 *
 * tfa_hal_open() creates plugin instance by tying plugin symbol delivered by user
 * with tfa_hal_device structure. During call init() method of plugin will be called.
 * When function succeeds, plugin is ready to use.
 *
 * @pre Please ensure that plug_symbol and functions pointed by it are accessible during app life time
 *
 * @param[in]  tfa    tfa device instance that uses plugin. It could be NULL, when there is none.
 * @param[in]  plug   plugin handle, usually returned by system service. Could be NULL, when plugin is static
 * @param[in]  plug_symbol instance of plugin
 * @param[out] dev    handle to plugin
 * @param[in]  param  plugin options (string)
 * @param[in]  len    plugin options length
 *
 * @return 0 when successful, error code otherwise @ref hal_error_codes.
 *
 */
int tfa_hal_open(struct tfa_device *tfa, const void *plug,
			struct tfa_hal_plugin_funcs *plug_symbol,
			struct tfa_hal_dev **dev, const char *param, size_t len);
/**
 * @brief Function ties plugin symbol with tfa_hal_device.
 *
 * tfa_hal_open_to_get_tfa_details() is similar to tfa_hal_open() function. It is used only for getting the TFA details in offline mode.
 * Any HAL level specific steps for initialization in offline mode can be handled here. Eg: For UDP, this API has lower UDP retries and timeouts
 * inorder to reduce the delay.
 * 
 * When function succeeds, plugin is ready to use.
 *
 * @pre Please ensure that plug_symbol and functions pointed by it are accessible during app life time
 *
 * @param[in]  tfa    tfa device instance that uses plugin. It could be NULL, when there is none.
 * @param[in]  plug   plugin handle, usually returned by system service. Could be NULL, when plugin is static
 * @param[in]  plug_symbol instance of plugin
 * @param[out] dev    handle to plugin
 * @param[in]  param  plugin options (string)
 * @param[in]  len    plugin options length
 *
 * @return 0 when successful, error code otherwise @ref hal_error_codes.
 *
 */
int tfa_hal_open_to_get_tfa_details(struct tfa_device *tfa, const void *plug,
		struct tfa_hal_plugin_funcs *plug_symbol,
		struct tfa_hal_dev **dev, const char *param, size_t len);


/**
 * @brief Function loads external plugin (dynamic library).
 *
 * tfa_hal_load() creates plugin instance by tying plugin symbol delivered from external dynamic library
 * with tfa_hal_device structure. During call init() method of plugin will be called.
 * When function succeeds, plugin is ready to use.
 *
 * @post External dynamic libraries will be loaded into process address space
 * @post Please ensure that plug_symbol and functions pointed by it are accessible during app life time
 *
 * @param[in]  tfa    tfa device instance that uses plugin. It could be NULL, when there is none.
 * @param[in]  plug   plugin handle, usually returned by system service. Could be NULL, when plugin is static
 * @param[in]  plug_symbol instance of plugin
 * @param[out] dev    handle to plugin
 * @param[in]  param  plugin options (string)
 * @param[in]  len    plugin options length
 *
 * @return 0 when successful, error code otherwise @ref hal_error_codes.
 *
 */
int tfa_hal_load(struct tfa_device *tfa, const char *target,
			struct tfa_hal_dev **dev, const char *param, size_t len);


/**
 * @brief Function loads two plug-ins in overlay configuration.
 *
 * tfa_hal_load_overlay() creates plugin instance by combining plug-ins symbol
 * with tfa_hal_device structure. During call init() method of plug-ins will be called.
 * When function succeeds, plug-ins system is ready to use.
 *
 * @post External dynamic libraries will be loaded into process address space
 * @post Please ensure that plug_symbol and functions pointed by it are accessible during app life time
 *
 * @param[in]  tfa     tfa device instance that uses plugin. It could be NULL, when there is none.
 * @param[out] dev     handle to plugin
 * @param[in]  plug_in main plug-in description
 * @param[in]  overlay overlay plug-in description

 * @return 0 when successful, error code otherwise @ref hal_error_codes.
 *
 */
int tfa_hal_load_overlay(struct tfa_device *tfa, struct tfa_hal_dev **dev,
                         struct tfa_hal_plug_desc *plug_in,
                         struct tfa_hal_plug_desc *overlay);

/**
 * @brief Function closes plugin opened by tfa_hal_open() or loaded by tfa_hal_load().
 *
 * tfa_hal_close() finalizes plugin instance by detaching plugin from system.
 * During this call close() method of plugin will be called.
 * When function succeeds, plugin is fully unloaded.
 *
 * @post External dynamic libraries will be un-loaded from process address space (external plugin only)
 * @post Any call to tfa_hal_dev instance, which called tfa_hal_close() will cause segmentation fault.
 *
 * @param[in] dev    handle to plugin
 *
 * @return 0 when successful, error code otherwise @ref hal_error_codes.
 *
 */
int tfa_hal_close(struct tfa_hal_dev *dev);

/**
 * @brief queries plugin about prefered buffer size during communication.
 *
 * @param[in]  dev      handle to plugin
 * @param[out] buf_size bufer size expressed as size_t
 *
 * @return 0 when successful, error code otherwise @ref hal_error_codes.
 *
 */
int tfa_hal_buffersize(struct tfa_hal_dev *dev, size_t *buf_size);

/**
 * @brief queries plugin scriboname.
 *
 * @param[in]  dev      handle to plugin
 * @param[out] buf      pointer to string buffer
 * @param[in]  len      size of string buffer
 *
 * @return 0 when successful, error code otherwise @ref hal_error_codes.
 *
 */
int tfa_hal_get_scriboname(struct tfa_hal_dev *dev, char *buf, size_t len);

/**
 * @brief queries plugin about supported interface type (please refer to @ref tfa_hal_iftype).
 *
 * @param[in]  dev      handle to plugin
 * @param[out] buf      pointer to string buffer
 * @param[in]  len      size of string buffer
 *
 * @return 0 when successful, error code otherwise @ref hal_error_codes.
 *
 */
int tfa_hal_get_iftype(struct tfa_hal_dev *dev);

/**
 * @brief transfers data to/from device.
 *
 * tfa_hal_transfer() is a main I/O function. It used only for I2C devices transfers.
 * Tfadsp-like devices are handled by messaging interface (@ref tfa_hal_tfadsp_execute).
 * It supports logic specific for I2C devices. Each operation consists of write operation
 * or write/read operation. To trigger write only operation out_buf and out_bytes paramters
 * should be set to 0.
 *
 * @param[in]  dev       handle to plugin
 * @param[in]  address   target device address (depends on plugin, container file)
 * @param[in]  in_buf    buffer containing data to be send to device
 * @param[in]  in_bytes  size of in_buf expressed in bytes
 * @param[out] out_buf   buffer to be filled with data received from device (R/W I/O only, NULL otherwise)
 * @param[in]  out_bytes size of out_buf expressed in bytes (R/W I/O only, 0 otherwise)
 *
 * @return 0 when successful, error code otherwise @ref hal_error_codes.
 *
 */
int tfa_hal_transfer(struct tfa_hal_dev *dev, unsigned address,
			const char *in_buf, unsigned in_bytes,
			char *out_buf, unsigned out_bytes);

/**
 * @brief gets value of pin.
 *
 * @param[in]  dev       handle to plugin
 * @param[in]  pin       pin number
 * @param[out] value     value read back from pin
 *
 * @return 0 when successful, error code otherwise @ref hal_error_codes.
 *
 */
int tfa_hal_get_pin(struct tfa_hal_dev *dev, int pin, int *value);

/**
 * @brief sets value of pin.
 *
 * @param[in]  dev       handle to plugin
 * @param[in]  pin       pin number
 * @param[out] value     value to set on pin
 *
 * @return 0 when successful, error code otherwise @ref hal_error_codes.
 *
 */
int tfa_hal_set_pin(struct tfa_hal_dev *dev, int pin, int value);

/**
 * @brief returns plugin version string.
 *
 * @param[in]  dev       handle to plugin
 * @param[out] buf       string buffer to store version
 * @param[in]  len     size of buf in bytes
 *
 * @return 0 when successful, error code otherwise @ref hal_error_codes.
 *
 */
int tfa_hal_version(struct tfa_hal_dev *dev, char *buf, size_t len);

/**
 * @brief stop playback on target.
 *
 * @param[in]  dev  handle to plugin
 *
 * @return 0 when successful, error code otherwise @ref hal_error_codes.
 *
 */
int tfa_hal_stopplayback(struct tfa_hal_dev *dev);

/**
 * @brief start playback on target, use file given in buffer 'buf'.
 *
 * @param[in]  dev       handle to plugin
 * @param[in]  buf  file name of audio file to playback
 * @param[in]  len  length of string stored in buf, expressed in bytes
 *
 * @return 0 when successful, error code otherwise @ref hal_error_codes.
 *
 */
int tfa_hal_startplayback(struct tfa_hal_dev *dev, const char *buf, size_t len);


/**
 * @brief transfers messages directed to tfadsp instance.
 *
 * tfa_hal_tfadsp_execute() is a main I/O function for tfadsp case.
 * It supports logic specific for tfadsp messaging.
 * Each operation consists of write, read operation or write/read operation.
 * No I2C transaction should be issued using this interface.
 *
 * In case of write only or read only operations some arguments could be set to NULL.
 *
 * @param[in]  dev     handle to plugin
 * @param[in]  address device uniqe identifier
 * @param[in]  cmd_buf command buffer (NULL for read-only operation)
 * @param[in]  cmd_len size of command buffer in bytes (0 for read-only operation)
 * @param[out] res_buf response buffer (NULL for write-only operation)
 * @param[in]  res_len response buffer size in bytes (0 for write-only operation)
 *
 * @return 0 when successful, error code otherwise @ref hal_error_codes.
 *
 */
int tfa_hal_tfadsp_execute(struct tfa_hal_dev *dev, unsigned address,
				const char *cmd_buf, size_t cmd_len,
				char *res_buf, size_t res_len);
/**
 * @brief transfers init message directed to tfadsp instance.
 *
 * Init message is special instance of message. For general messaging @ref tfa_hal_tfadsp_execute
 * should be used. Init message goal is to send set of params to startup tfadsp-like target.
 * This call in special conditions could mean restart(the call is done during normal operation)
 * However this is plugin dependent behviour.
 *
 * @param[in]  dev     handle to plugin
 * @param[in]  address device uniqe identifier
 * @param[in]  buf command buffer (NULL for read-only operation)
 * @param[in]  len size of command buffer in bytes (0 for read-only operation)
 *
 * @return 0 when successful, error code otherwise @ref hal_error_codes.
 *
 */
int tfa_hal_tfadsp_init(struct tfa_hal_dev *dev, unsigned address,
			const char *buf, size_t len);


/**
 * @brief select file that HAL layer traces into (could be stdout).
 *
 * @remark this operation is device independent. It could be called always.
 *
 * @param[in]  io handle to output stream
 *
 * @return 0 when successful, error code otherwise @ref hal_error_codes.
 *
 */
int tfa_hal_set_tracefile(FILE *io);

/**
 * @brief select file that HAL layer uses to output logging messages into.
 *
 * @remark this operation is device independent. It could be called always.
 *
 * @param[in]  io handle to output stream (Can't be NULL)
 *
 * @return 0 when successful, error code otherwise @ref hal_error_codes.
 *
 */
int tfa_hal_set_logfile(FILE *io);


/* Set of bit masks used to set and get log_mask settings */
#define TFA_HAL_TRACE                    0x01  /**< Enables HAL tracing */
#define TFA_HAL_PLUGIN_VERBOSITY         0x0E  /**< Enables HAL plugins verbosity */
#define TFA_HAL_VERBOSITY                0x10  /**< Enables HAL verbosity */

/** log_mask is a bitfield which contains settings for tracing and
 *  logging provided by HAL module and all plugins supported by HAL.
 *
 *  Bit meanings:
 *  Bit 0 -> Tracing on/off (1 for ON)
 *  Bit 1 -> Plugins logging (verbosity) level.
 *  Bit 4 -> HAL module logging on/off (1 for ON). Currently not used.
 */

/**
 * @brief start playback on target, use file given in buffer 'buf'.
 *
 * @param[in]  dev       handle to plugin
 * @param[in]  buf  file name of audio file to playback
 * @param[in]  len  length of string stored in buf, expressed in bytes
 *
 * @return 0 when successful, error code otherwise @ref hal_error_codes.
 *
 */
int tfa_hal_set_trace(unsigned mask);

/**
 * @brief returns plug-in symbol based in name for plug-ins delivered with HAL.
 *
 * @param[in] plug_in name of plug-in
 *
 * @return Plug-in symbol pointer or NULL when not found.
 *
 */
struct tfa_hal_plugin_funcs *tfa_hal_get_plugin_symbol(const char *plug_name);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* TFA_HAL_H */
