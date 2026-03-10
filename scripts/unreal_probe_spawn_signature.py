import unreal
sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
unreal.log('[ProbeSig] ' + str(sub.spawn_actor_from_class.__doc__))
