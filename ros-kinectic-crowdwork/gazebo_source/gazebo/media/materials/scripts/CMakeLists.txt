set (files
blur.compositor
blur.material
deferred.compositor
deferred_post.material
deferred_post_minilight.material
deferred_post.program
distortion.compositor
Examples.program
GameChar_Male.material
gazebo.material
grid.material
kitchen.material
noise.compositor
oculus.material
perpixel.program
picker.material
ShadowCaster.material
shadow_caster.program
ShadowCaster.program
ssao.compositor
ssao.material
VPL.material
VPL.program
WideCameraLensMap.compositor
)

install(FILES ${files} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/gazebo-${GAZEBO_MAJOR_VERSION}/media/materials/scripts/)

