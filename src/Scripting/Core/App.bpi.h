
class_<App, noncopyable>("App", no_init)
	.def("getScene", &App::getScene, return_value_policy<reference_existing_object>());
