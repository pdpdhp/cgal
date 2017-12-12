


// Initializes the animation parameters:
//   1. Chooses two points `source` and `target` at random
//   2. Chooses a `time` parameter for which the point on the segment [source, target] lies inside the octagon
//   3. Locates this point in the triangulation and keeps the location face and translation
void 
MainWindow::initialize_animation_parameters() {

	timestep = 0.005;

	std::srand(std::time(0));
	double rx1, rx2, ry1, ry2;
	rx1 = ((double)std::rand()/(double)RAND_MAX)*0.6;
	rx2 = ((double)std::rand()/(double)RAND_MAX)*0.6;
	ry1 = ((double)std::rand()/(double)RAND_MAX)*0.6;
	ry2 = ((double)std::rand()/(double)RAND_MAX)*0.6;
	source = Point(rx1, ry1);
	target = Point(rx2, ry2);
	Segment_2 seg = Construct_hyperbolic_line_2()(source, target);
	Circular_arc_2* carc = boost::get<Circular_arc_2>(&seg);
	source = carc->source();
	target = carc->target();

	time = updateTime();
	last_location = dt.periodic_locate(get_image(source, target, time), last_loc_translation); 
}


// Gets all the translations of the set N (neighbors of the octagon)
template < typename NT >
void get_all_translations(std::vector< CGAL::Hyperbolic_octagon_translation<NT> >& tr) {
	typedef CGAL::Hyperbolic_octagon_translation<NT> Translation;
	tr.push_back(Translation(0,5,2,7));
	tr.push_back(Translation(0,5,2));
	tr.push_back(Translation(0,5));
	tr.push_back(Translation(0));
	tr.push_back(Translation(0,3));
	tr.push_back(Translation(0,3,6));

	tr.push_back(Translation(1,6,3,0));
	tr.push_back(Translation(1,6,3));
	tr.push_back(Translation(1,6));
	tr.push_back(Translation(1));
	tr.push_back(Translation(1,4));
	tr.push_back(Translation(1,4,7));

	tr.push_back(Translation(2,7,4,1));
	tr.push_back(Translation(2,7,4));
	tr.push_back(Translation(2,7));
	tr.push_back(Translation(2));
	tr.push_back(Translation(2,5));
	tr.push_back(Translation(2,5,0));

	tr.push_back(Translation(3,0,5,2));
	tr.push_back(Translation(3,0,5));
	tr.push_back(Translation(3,0));
	tr.push_back(Translation(3));
	tr.push_back(Translation(3,6));
	tr.push_back(Translation(3,6,1));

	tr.push_back(Translation(4,1,6,3));
	tr.push_back(Translation(4,1,6));
	tr.push_back(Translation(4,1));
	tr.push_back(Translation(4));
	tr.push_back(Translation(4,7));
	tr.push_back(Translation(4,7,2));

	tr.push_back(Translation(5,2,7,4));
	tr.push_back(Translation(5,2,7));
	tr.push_back(Translation(5,2));
	tr.push_back(Translation(5));
	tr.push_back(Translation(5,0));
	tr.push_back(Translation(5,0,3));

	tr.push_back(Translation(6,3,0,5));
	tr.push_back(Translation(6,3,0));
	tr.push_back(Translation(6,3));
	tr.push_back(Translation(6));
	tr.push_back(Translation(6,1));
	tr.push_back(Translation(6,1,4));

	tr.push_back(Translation(7,4,1,6));
	tr.push_back(Translation(7,4,1));
	tr.push_back(Translation(7,4));
	tr.push_back(Translation(7));
	tr.push_back(Translation(7,2));
	tr.push_back(Translation(7,2,5));
}


// calculates the point on the hyperbolic segment [src, tgt] for parameter `time`.
Point
MainWindow::get_image(Point src, Point tgt, double time) {
  Segment_2 seg = Construct_hyperbolic_segment_2()(src, tgt);
  Circular_arc_2* carc = boost::get<Circular_arc_2>(&seg);
  Circle_2 crc = carc->circle();
  
  double sx = to_double(((src.x()) - crc.center().x())/sqrt(crc.squared_radius()));
  double sy = to_double(((src.y()) - crc.center().y())/sqrt(crc.squared_radius()));
  double tx = to_double(((tgt.x()) - crc.center().x())/sqrt(crc.squared_radius()));
  double ty = to_double(((tgt.y()) - crc.center().y())/sqrt(crc.squared_radius()));
  
  double dot = to_double(sx*tx + sy*ty);
  double n1 = sqrt(sx*sx+sy*sy);
  double n2 = sqrt(tx*tx+ty*ty);
  double theta = acos(dot/n1/n2);

  double x = sin((1.0-time)*theta)/sin(theta)*sx + sin(time*theta)/sin(theta)*tx;
  double y = sin((1.0-time)*theta)/sin(theta)*sy + sin(time*theta)/sin(theta)*ty;
  
  x = to_double(x*sqrt(crc.squared_radius()) + crc.center().x());
  y = to_double(y*sqrt(crc.squared_radius()) + crc.center().y());

  Point p(x, y);

  return p;
}


// Updated the `time` so that the corresponding point on the segment [source, target]
// lies inside the octagon.
double 
MainWindow::updateTime() {
  double t = 0.1;
  Side_of_original_octagon check;
  while(check(get_image(source, target, t)) == CGAL::ON_UNBOUNDED_SIDE) {
    t += timestep;
  }
  return t;
}


// Main animation function.
void
MainWindow::animate() {
  
  Point p = get_image(source, target, time);

  Side_of_original_octagon check;

  // If the point is inside the octagon, just increment the time, compute the point 
  // corresponding to that time, update everything, and continue.
  if (check(p) == CGAL::ON_BOUNDED_SIDE) {
    Locate_type lt;
    int li;
    last_location = dt.periodic_locate(p, lt, li, last_loc_translation, last_location);
    dgi->setMovingPoint(p);
    dgi->setSource(source);
    dgi->setTarget(target);
    
    time += timestep;

  } 
  // If the point is not inside the octagon, we must find the translation that puts 
  // it inside the octagon. We apply the same translation to the `source` and `target`.
  else { 

    Hyperbolic_translation o; 

    bool found = false;

    // Look for the translation that gets p inside the octagon among
    // the generators of the group. The correct translation will be
    // in this set with high probability.
    std::vector<Hyperbolic_translation> gens;
    get_generators(gens);
    for (int i = 0; i < gens.size(); i++) {
      o = gens[i];
      if (check(Make_point()(p,o)) == CGAL::ON_BOUNDED_SIDE) {
        found = true;
        break;
      }
    }

    // If the correct translation is NOT one of the generators, it will
    // be among the translations corresponding to the neighboring regions
    // of the octagon. Check them all.
    if (!found) {
      std::vector<Hyperbolic_translation> tr;
      get_all_translations(tr);
      for (int i = 0; i < tr.size(); i++) {
        o = tr[i];
        if (check(Make_point()(p,o)) == CGAL::ON_BOUNDED_SIDE) {
          found = true;
          break;
        }
      }
    }

    // The correct translation MUST have been identified now.
 	CGAL_assertion(found);

    source = Make_point()(source,o);
    target = Make_point()(target,o);
    
    // Correct in case of wrong orientation.
    Segment_2 seg = Construct_hyperbolic_line_2()(source, target);
    Circular_arc_2* carc = boost::get<Circular_arc_2>(&seg);
    if (sqrt(squared_distance(source, carc->source())) < sqrt(squared_distance(source, carc->target()))) {
      source = carc->source();
      target = carc->target();
    } else {
      source = carc->target();
      target = carc->source();
    }

    // Make sure the time is correct.
    time = updateTime();
  }

  // Cause the window to redraw itself
  emit(changed());
  qApp->processEvents();

  // This variable reflects if the animation button in the demo window is active or not.
  if (go) {
    // Uncomment here to generate snapshots!
    // QPixmap sshot = this->grab(QRect(QPoint(560, 210), QSize(780, 770)));
    // //QPixmap sshot = this->grab(QRect(QPoint(320, 55), QSize(625, 615)));
    // std::stringstream ss;
    // ss << "/Users/iordanov/Desktop/shots/sshot" << (idx++) << ".png"; // <--- use your own path here!
    // sshot.save(QString(ss.str().c_str()));
    
    // Recursive call
    animate();
  }
}
