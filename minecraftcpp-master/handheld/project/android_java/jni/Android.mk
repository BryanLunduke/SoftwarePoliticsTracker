LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := minecraftpe
LOCAL_SRC_FILES := ../../../src/main.cpp \
                   ../../../src/main_android_java.cpp \
                   ../../../src/platform/input/Controller.cpp \
                   ../../../src/platform/input/Keyboard.cpp \
                   ../../../src/platform/input/Mouse.cpp \
                   ../../../src/platform/input/Multitouch.cpp \
                   ../../../src/platform/time.cpp \
                   ../../../src/platform/CThread.cpp \
../../../src/NinecraftApp.cpp \
../../../src/Performance.cpp \
../../../src/SharedConstants.cpp \
../../../src/client/IConfigListener.cpp \
../../../src/client/Minecraft.cpp \
../../../src/client/MouseHandler.cpp \
../../../src/client/Options.cpp \
../../../src/client/OptionStrings.cpp \
../../../src/client/gamemode/GameMode.cpp \
../../../src/client/gamemode/CreativeMode.cpp \
../../../src/client/gamemode/SurvivalMode.cpp \
../../../src/client/gui/components/Button.cpp \
../../../src/client/gui/components/ImageButton.cpp \
../../../src/client/gui/components/ItemPane.cpp \
../../../src/client/gui/components/InventoryPane.cpp \
../../../src/client/gui/components/LargeImageButton.cpp \
../../../src/client/gui/components/RolledSelectionListH.cpp \
../../../src/client/gui/components/RolledSelectionListV.cpp \
../../../src/client/gui/components/ScrolledSelectionList.cpp \
../../../src/client/gui/components/ScrollingPane.cpp \
../../../src/client/gui/components/SmallButton.cpp \
../../../src/client/gui/Font.cpp \
../../../src/client/gui/Gui.cpp \
../../../src/client/gui/GuiComponent.cpp \
../../../src/client/gui/Screen.cpp \
../../../src/client/gui/screens/ScreenChooser.cpp \
../../../src/client/gui/screens/ChatScreen.cpp \
../../../src/client/gui/screens/ChestScreen.cpp \
../../../src/client/gui/screens/ConfirmScreen.cpp \
../../../src/client/gui/screens/DeathScreen.cpp \
../../../src/client/gui/screens/FurnaceScreen.cpp \
../../../src/client/gui/screens/InBedScreen.cpp \
../../../src/client/gui/screens/IngameBlockSelectionScreen.cpp \
../../../src/client/gui/screens/JoinGameScreen.cpp \
../../../src/client/gui/screens/OptionsScreen.cpp \
../../../src/client/gui/screens/PauseScreen.cpp \
../../../src/client/gui/screens/ProgressScreen.cpp \
../../../src/client/gui/screens/RenameMPLevelScreen.cpp \
../../../src/client/gui/screens/SelectWorldScreen.cpp \
../../../src/client/gui/screens/StartMenuScreen.cpp \
../../../src/client/gui/screens/TextEditScreen.cpp \
../../../src/client/gui/screens/touch/TouchIngameBlockSelectionScreen.cpp \
../../../src/client/gui/screens/touch/TouchJoinGameScreen.cpp \
../../../src/client/gui/screens/touch/TouchSelectWorldScreen.cpp \
../../../src/client/gui/screens/touch/TouchStartMenuScreen.cpp \
../../../src/client/gui/screens/UploadPhotoScreen.cpp \
../../../src/client/gui/screens/crafting/PaneCraftingScreen.cpp \
../../../src/client/gui/screens/crafting/WorkbenchScreen.cpp \
../../../src/client/model/ChickenModel.cpp \
../../../src/client/model/CowModel.cpp \
../../../src/client/model/HumanoidModel.cpp \
../../../src/client/model/PigModel.cpp \
../../../src/client/model/SheepFurModel.cpp \
../../../src/client/model/SheepModel.cpp \
../../../src/client/model/QuadrupedModel.cpp \
../../../src/client/model/geom/Cube.cpp \
../../../src/client/model/geom/ModelPart.cpp \
../../../src/client/model/geom/Polygon.cpp \
../../../src/client/particle/Particle.cpp \
../../../src/client/particle/ParticleEngine.cpp \
../../../src/client/player/LocalPlayer.cpp \
../../../src/client/player/RemotePlayer.cpp \
../../../src/client/player/input/KeyboardInput.cpp \
../../../src/client/player/input/touchscreen/TouchscreenInput.cpp \
../../../src/client/renderer/Chunk.cpp \
../../../src/client/renderer/EntityTileRenderer.cpp \
../../../src/client/renderer/GameRenderer.cpp \
../../../src/client/renderer/ItemInHandRenderer.cpp \
../../../src/client/renderer/LevelRenderer.cpp \
../../../src/client/renderer/RenderChunk.cpp \
../../../src/client/renderer/RenderList.cpp \
../../../src/client/renderer/Tesselator.cpp \
../../../src/client/renderer/Textures.cpp \
../../../src/client/renderer/TileRenderer.cpp \
../../../src/client/renderer/gles.cpp \
../../../src/client/renderer/culling/Frustum.cpp \
../../../src/client/renderer/entity/ArrowRenderer.cpp \
../../../src/client/renderer/entity/ChickenRenderer.cpp \
../../../src/client/renderer/entity/EntityRenderDispatcher.cpp \
../../../src/client/renderer/entity/EntityRenderer.cpp \
../../../src/client/renderer/entity/HumanoidMobRenderer.cpp \
../../../src/client/renderer/entity/ItemRenderer.cpp \
../../../src/client/renderer/entity/ItemSpriteRenderer.cpp \
../../../src/client/renderer/entity/MobRenderer.cpp \
../../../src/client/renderer/entity/PaintingRenderer.cpp \
../../../src/client/renderer/entity/PlayerRenderer.cpp \
../../../src/client/renderer/entity/SheepRenderer.cpp \
../../../src/client/renderer/entity/TntRenderer.cpp \
../../../src/client/renderer/entity/TripodCameraRenderer.cpp \
../../../src/client/renderer/ptexture/DynamicTexture.cpp \
../../../src/client/renderer/tileentity/ChestRenderer.cpp \
../../../src/client/renderer/tileentity/SignRenderer.cpp \
../../../src/client/renderer/tileentity/TileEntityRenderDispatcher.cpp \
../../../src/client/renderer/tileentity/TileEntityRenderer.cpp \
../../../src/client/sound/Sound.cpp \
../../../src/client/sound/SoundEngine.cpp \
../../../src/locale/I18n.cpp \
../../../src/nbt/Tag.cpp \
../../../src/network/command/CommandServer.cpp \
../../../src/network/ClientSideNetworkHandler.cpp \
../../../src/network/NetEventCallback.cpp \
../../../src/network/Packet.cpp \
../../../src/network/RakNetInstance.cpp \
../../../src/network/ServerSideNetworkHandler.cpp \
../../../src/server/ServerLevel.cpp \
../../../src/server/ServerPlayer.cpp \
../../../src/util/DataIO.cpp \
../../../src/util/Mth.cpp \
../../../src/util/StringUtils.cpp \
../../../src/util/PerfTimer.cpp \
../../../src/util/PerfRenderer.cpp \
../../../src/world/Direction.cpp \
../../../src/world/entity/AgableMob.cpp \
../../../src/world/entity/Entity.cpp \
../../../src/world/entity/EntityFactory.cpp \
../../../src/world/entity/FlyingMob.cpp \
../../../src/world/entity/HangingEntity.cpp \
../../../src/world/entity/Mob.cpp \
../../../src/world/entity/MobCategory.cpp \
../../../src/world/entity/Motive.cpp \
../../../src/world/entity/Painting.cpp \
../../../src/world/entity/PathfinderMob.cpp \
../../../src/world/entity/SynchedEntityData.cpp \
../../../src/world/entity/ai/control/MoveControl.cpp \
../../../src/world/entity/animal/Animal.cpp \
../../../src/world/entity/animal/Chicken.cpp \
../../../src/world/entity/animal/Cow.cpp \
../../../src/world/entity/animal/Pig.cpp \
../../../src/world/entity/animal/Sheep.cpp \
../../../src/world/entity/animal/WaterAnimal.cpp \
../../../src/world/entity/item/FallingTile.cpp \
../../../src/world/entity/item/ItemEntity.cpp \
../../../src/world/entity/item/PrimedTnt.cpp \
../../../src/world/entity/item/TripodCamera.cpp \
../../../src/world/entity/monster/Creeper.cpp \
../../../src/world/entity/monster/Monster.cpp \
../../../src/world/entity/monster/PigZombie.cpp \
../../../src/world/entity/monster/Skeleton.cpp \
../../../src/world/entity/monster/Spider.cpp \
../../../src/world/entity/monster/Zombie.cpp \
../../../src/world/entity/projectile/Arrow.cpp \
../../../src/world/entity/projectile/Throwable.cpp \
../../../src/world/entity/player/Inventory.cpp \
../../../src/world/entity/player/Player.cpp \
../../../src/world/food/SimpleFoodData.cpp \
../../../src/world/inventory/BaseContainerMenu.cpp \
../../../src/world/inventory/ContainerMenu.cpp \
../../../src/world/inventory/FillingContainer.cpp \
../../../src/world/inventory/FurnaceMenu.cpp \
../../../src/world/item/BedItem.cpp \
../../../src/world/item/DyePowderItem.cpp \
../../../src/world/item/Item.cpp \
../../../src/world/item/ItemInstance.cpp \
../../../src/world/item/HangingEntityItem.cpp \
../../../src/world/item/HatchetItem.cpp \
../../../src/world/item/HoeItem.cpp \
../../../src/world/item/PickaxeItem.cpp \
../../../src/world/item/ShovelItem.cpp \
../../../src/world/item/crafting/Recipe.cpp \
../../../src/world/item/crafting/Recipes.cpp \
../../../src/world/item/crafting/FurnaceRecipes.cpp \
../../../src/world/item/crafting/OreRecipes.cpp \
../../../src/world/item/crafting/StructureRecipes.cpp \
../../../src/world/item/crafting/ToolRecipes.cpp \
../../../src/world/item/crafting/WeaponRecipes.cpp \
../../../src/world/level/Explosion.cpp \
../../../src/world/level/Level.cpp \
../../../src/world/level/LightLayer.cpp \
../../../src/world/level/LightUpdate.cpp \
../../../src/world/level/MobSpawner.cpp \
../../../src/world/level/Region.cpp \
../../../src/world/level/TickNextTickData.cpp \
../../../src/world/level/biome/Biome.cpp \
../../../src/world/level/biome/BiomeSource.cpp \
../../../src/world/level/chunk/LevelChunk.cpp \
../../../src/world/level/dimension/Dimension.cpp \
../../../src/world/level/levelgen/CanyonFeature.cpp \
../../../src/world/level/levelgen/DungeonFeature.cpp \
../../../src/world/level/levelgen/LargeCaveFeature.cpp \
../../../src/world/level/levelgen/LargeFeature.cpp \
../../../src/world/level/levelgen/RandomLevelSource.cpp \
../../../src/world/level/levelgen/feature/Feature.cpp \
../../../src/world/level/levelgen/synth/ImprovedNoise.cpp \
../../../src/world/level/levelgen/synth/PerlinNoise.cpp \
../../../src/world/level/levelgen/synth/Synth.cpp \
../../../src/world/level/material/Material.cpp \
../../../src/world/level/pathfinder/Path.cpp \
../../../src/world/level/storage/ExternalFileLevelStorage.cpp \
../../../src/world/level/storage/ExternalFileLevelStorageSource.cpp \
../../../src/world/level/storage/FolderMethods.cpp \
../../../src/world/level/storage/LevelData.cpp \
../../../src/world/level/storage/LevelStorageSource.cpp \
../../../src/world/level/storage/RegionFile.cpp \
../../../src/world/level/tile/BedTile.cpp \
../../../src/world/level/tile/ChestTile.cpp \
../../../src/world/level/tile/CropTile.cpp \
../../../src/world/level/tile/DoorTile.cpp \
../../../src/world/level/tile/EntityTile.cpp \
../../../src/world/level/tile/FurnaceTile.cpp \
../../../src/world/level/tile/GrassTile.cpp \
../../../src/world/level/tile/LightGemTile.cpp \
../../../src/world/level/tile/MelonTile.cpp \
../../../src/world/level/tile/Mushroom.cpp \
../../../src/world/level/tile/NetherReactor.cpp \
../../../src/world/level/tile/NetherReactorPattern.cpp \
../../../src/world/level/tile/SandTile.cpp \
../../../src/world/level/tile/StemTile.cpp \
../../../src/world/level/tile/StoneSlabTile.cpp \
../../../src/world/level/tile/TallGrass.cpp \
../../../src/world/level/tile/Tile.cpp \
../../../src/world/level/tile/TrapDoorTile.cpp \
../../../src/world/level/tile/entity/ChestTileEntity.cpp \
../../../src/world/level/tile/entity/NetherReactorTileEntity.cpp \
../../../src/world/level/tile/entity/SignTileEntity.cpp \
../../../src/world/level/tile/entity/TileEntity.cpp \
../../../src/world/level/tile/entity/FurnaceTileEntity.cpp \
../../../src/world/phys/HitResult.cpp

LOCAL_CFLAGS := -Wno-psabi $(LOCAL_CFLAGS)

LOCAL_CFLAGS := -DPRE_ANDROID23 -DANDROID_PUBLISH $(LOCAL_CFLAGS)
#LOCAL_CFLAGS := -DPRE_ANDROID23 -DANDROID_PUBLISH -DDEMO_MODE $(LOCAL_CFLAGS)

#LOCAL_CFLAGS := -DPRE_ANDROID23 -DDEMO_MODE $(LOCAL_CFLAGS)
#LOCAL_CFLAGS := -DPRE_ANDROID23 $(LOCAL_CFLAGS)

LOCAL_LDLIBS    := -llog -lEGL -lGLESv1_CM

LOCAL_STATIC_LIBRARIES := RakNet

TARGET_ARCH_ABI := armeabi-v7a

include $(BUILD_SHARED_LIBRARY)

# NOTE: environment var NDK_MODULE_PATH needs to point to lib_projects folder
$(call import-module, raknet/jni)

