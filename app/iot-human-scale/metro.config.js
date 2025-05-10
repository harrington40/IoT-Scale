// metro.config.js
const { getDefaultConfig } = require("metro-config");

module.exports = (async () => {
  const {
    resolver: { assetExts }
  } = await getDefaultConfig();

  return {
    resolver: {
      // add 'webp' to the list of asset extensions
      assetExts: [...assetExts, "webp"]
    }
  };
})();
