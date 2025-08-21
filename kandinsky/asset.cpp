#include <kandinsky/asset.h>

#include <kandinsky/platform.h>
#include <kandinsky/core/serde.h>

namespace kdk {

void SerializeAssetHandle(SerdeArchive* sa, EAssetType type, AssetHandle* handle) {
	(void)sa;
	(void)type;
	(void)handle;
	ASSERTF(false, "Not implemented yet");
}

}  // namespace kdk
