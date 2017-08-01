#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_fdt.h>
#include <linux/firmware.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/slab.h>

#define VERSION			"1.0"

struct em_overlay_info {
	char			*filename;
	struct list_head	list;
};

struct expansion_manager {
	struct mutex		lock;
	struct list_head	overlays;
};

static int load_overlay(struct device *dev, char *filename) {
	const struct firmware *fw;
	struct device_node *overlay;
	int err;

	fw = kzalloc(sizeof(struct firmware), GFP_KERNEL);

	err = request_firmware_direct(&fw, filename, dev);
	if (err != 0) {
		dev_dbg(dev, "failed to load firmware '%s'\n", filename);
		err = -ENOENT;
		goto err_fail;
	}

	of_fdt_unflatten_tree((unsigned long *)fw->data, &overlay);
	if (overlay == NULL) {
		dev_err(dev, "Failed to unflatten\n");
		err = -EINVAL;
		goto err_fail;
	}

	of_node_set_flag(overlay, OF_DETACHED);

	err = of_resolve_phandles(overlay);
	if (err != 0) {
		dev_err(dev, "Failed to resolve tree\n");
		goto err_fail;
	}

	err = of_overlay_create(overlay);
	if (err < 0) {
		dev_err(dev, "Failed to create overlay\n");
		goto err_fail;
	}

	dev_info(dev, "Overlay '%s' loaded\n", filename);
	return 0;

err_fail:
	release_firmware(fw);
	return err;
}

static ssize_t em_overlays_show(struct device *dev,
		struct device_attribute *attr, char *buf) {
	struct expansion_manager *expansion_manager = dev_get_drvdata(dev);
	struct em_overlay_info *info;
	int count = 0;

	mutex_lock(&expansion_manager->lock);
	list_for_each_entry(info, &expansion_manager->overlays, list) {
		count += sprintf(&buf[count], "%s\n", info->filename);
	}
	mutex_unlock(&expansion_manager->lock);

	return count;
}

static ssize_t em_overlays_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count) {
	struct expansion_manager *expansion_manager = dev_get_drvdata(dev);
	struct em_overlay_info *info;
	int lenght, err;

	for (lenght = 0; lenght < count; lenght++) {
		if (buf[lenght] == '\0' || buf[lenght] == '\n') {
			lenght++;
			break;
		}
	}
	if (buf[lenght - 1] != '\0' && buf[lenght - 1] != '\n') {
		lenght++;
	}

	mutex_lock(&expansion_manager->lock);
	info = devm_kzalloc(dev, sizeof(struct em_overlay_info),GFP_KERNEL);
	if (!info) {
		err = -ENOMEM;
		goto exit_fail;
	}
	info->filename = devm_kzalloc(dev, lenght, GFP_KERNEL);
	if (!info->filename) {
		devm_kfree(dev, info);
		err = -ENOMEM;
		goto exit_fail;
	}
	memcpy(info->filename, buf, lenght - 1);
	info->filename[lenght - 1] = '\0';
	INIT_LIST_HEAD(&info->list);
	err = load_overlay(dev, info->filename);
	if (err) {
		devm_kfree(dev, info->filename);
		devm_kfree(dev, info);
		goto exit_fail;
	}
	list_add_tail(&info->list, &expansion_manager->overlays);
	err = count;
exit_fail:
	mutex_unlock(&expansion_manager->lock);

	return err;
}

static DEVICE_ATTR(overlays, S_IRUSR | S_IWUSR,
		em_overlays_show, em_overlays_store);

static int expansion_manager_probe(struct platform_device *pdev) {
	struct expansion_manager *expansion_manager;
	int ret;

	expansion_manager = devm_kzalloc(&pdev->dev,
				sizeof(struct expansion_manager), GFP_KERNEL);
	if (!expansion_manager)
		return -ENOMEM;

	mutex_init(&expansion_manager->lock);
	INIT_LIST_HEAD(&expansion_manager->overlays);
	dev_set_drvdata(&pdev->dev, expansion_manager);

	ret = device_create_file(&pdev->dev, &dev_attr_overlays);
	if (ret) {
		dev_err(&pdev->dev, "Impossible to create files under sysfs\n");
		return ret;
	}
	dev_info(&pdev->dev, "Version %s\n", VERSION);
	return ret;
}

static const struct of_device_id expansion_manager_of_match[] = {
	{
		.compatible = "bytesnap,expansion-manager",
	},
	{ },
};
MODULE_DEVICE_TABLE(of, expansion_manager_of_match);

static struct platform_driver expansion_manager_driver = {
	.probe = expansion_manager_probe,
	.driver = {
		.name = "expansion_manager",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(expansion_manager_of_match),
	},
};

static int __init expansion_driver_init(void) {
	return platform_driver_register(&expansion_manager_driver);
}
module_init(expansion_driver_init);

MODULE_AUTHOR("Fabrizio Castro <fabrizioc@bytesnap.co.uk>");
MODULE_DESCRIPTION("Expansion boards manager");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:expansion_manager");
