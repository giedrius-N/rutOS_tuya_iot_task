include $(TOPDIR)/rules.mk

PKG_NAME:=tuya_iot
PKG_RELEASE:=1
PKG_VERSION:=1.4.46

include $(INCLUDE_DIR)/package.mk

define Package/tuya_iot
	CATEGORY:=Extra packages
	TITLE:=tuya_iot
	DEPENDS:=+libtuyasdk +libubus +libubox +libblobmsg-json +libuci
endef

define Package/tuya_iot/description
	This is connection to Tuya services package
endef

define Package/tuya_iot/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tuya_iot $(1)/usr/bin
	$(INSTALL_BIN) ./files/tuya_iot.init $(1)/etc/init.d/tuya_iot
	$(INSTALL_CONF) ./files/tuya_iot.config $(1)/etc/config/tuya_iot

endef

$(eval $(call BuildPackage,tuya_iot))
