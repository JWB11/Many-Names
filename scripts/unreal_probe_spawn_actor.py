import unreal

sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
cls = unreal.ManyNamesScenicActor.static_class()
actor = sub.spawn_actor_from_class(cls, unreal.Vector(0.0, 0.0, 100.0), unreal.Rotator(0.0, 0.0, 0.0), True)
unreal.log('[ProbeSpawn] actor=' + str(actor))
