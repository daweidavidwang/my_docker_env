set (files
  beigeWall.jpg
  building_frame.png
  bricks.png
  ceiling_tiled.jpg
  clouds.jpg
  com.png
  dirt_diffusespecular.png
  dirt_normal.png
  flat_normal.png
  fungus_diffusespecular.png
  fungus_normal.png
  gameTex_0.png
  gameTex_11.png  gameTex_13.png  gameTex_2.png  gameTex_4.png  gameTex_6.png  gameTex_8.png
  gameTex_10.png  gameTex_12.png  gameTex_1.png   gameTex_3.png  gameTex_5.png  gameTex_7.png  gameTex_9.png

  granite2.jpg
  granite.jpg
  grass_diffusespecular.png
  grass.jpg
  grass_normal.png
  hardwood_floor.jpg
  heightmap_bowl.png
  motorway.jpg
  paintedWall.jpg
  pioneerBody.jpg
  primary.jpg
  projection_filter.png
  random.png
  residential.jpg
  road1.jpg
  runway.png
  secondary.jpg
  sidewalk.jpg
  steps.jpeg
  stereo_projection_pattern_high_res.png
  stereo_projection_pattern_high_res_red.png
  terrain_detail.jpg
  terrain.png
  trunk.jpg
  white.bmp
  willowMap.png
  wood.jpg
  WoodPallet.png
)


INSTALL(FILES ${files} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/gazebo-${GAZEBO_MAJOR_VERSION}/media/materials/textures/)
