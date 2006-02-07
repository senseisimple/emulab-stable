all: test-heap graph2dot test-dijk test-dijk.opt test-dre test-dre.opt \
	dre-histogram dre-histogram.opt name-ga name-ga.opt ordering-to-tree \
	ordering-to-tree.opt graph-to-tree graph-to-tree.opt tree-assign tree-assign.opt \
	graph2dot.opt

%.cmx: %.ml
	ocamlopt -c $<

%.cmo: %.ml $(if -e %.mli,%.cmi)
	ocamlc -g -c $<

%.cmo: %.ml
	ocamlc -g -c $<

%.cmi: %.mli
	ocamlc -g -c $<

graph2dot: graph2dot.ml graph.cmo
	ocamlc -g -o graph2dot str.cma graph.cmo graph2dot.ml

graph2dot.opt: graph2dot.ml graph.cmx
	ocamlopt -o graph2dot.opt str.cmxa graph.cmx graph2dot.ml

test-heap: test-heap.ml heap.cmo heap.cmi
	ocamlc -g -o test-heap heap.cmo test-heap.ml

test-dijk: test-dijk.ml heap.cmo heap.cmi graph.cmo dijkstra.cmo
	ocamlc -g -o test-dijk heap.cmo str.cma graph.cmo dijkstra.cmo /usr/local/lib/ocaml/str.cma test-dijk.ml

test-dijk.opt: heap.cmx graph.cmx dijkstra.cmx test-dijk.ml
	ocamlopt -o $@ /usr/local/lib/ocaml/str.cmxa $^

test-dre: heap.cmo graph.cmo dijkstra.cmo myset.cmo dre.cmo test-dre.ml
	ocamlc -g -o $@ /usr/local/lib/ocaml/str.cma $^

test-dre.opt: heap.cmx graph.cmx dijkstra.cmx myset.cmx dre.cmx test-dre.ml
	ocamlopt -o $@ /usr/local/lib/ocaml/str.cmxa $^

dre-histogram: heap.cmo graph.cmo dijkstra.cmo myset.cmo dre.cmo histogram.cmo dre-histogram.ml
	ocamlc -g -o $@ /usr/local/lib/ocaml/str.cma $^

dre-histogram.opt: heap.cmx graph.cmx dijkstra.cmx myset.cmx dre.cmx histogram.cmx dre-histogram.ml
	ocamlopt -o $@ /usr/local/lib/ocaml/str.cmxa $^

count-alternations: heap.cmo graph.cmo dijkstra.cmo myset.cmo dre.cmo histogram.cmo count-alternations.ml
	ocamlc -g -o $@ /usr/local/lib/ocaml/str.cma $^

count-alternations.opt: heap.cmx graph.cmx dijkstra.cmx myset.cmx dre.cmx histogram.cmx count-alternations.ml
	ocamlopt -o $@ /usr/local/lib/ocaml/str.cmxa $^

name-ga: heap.cmo graph.cmo dijkstra.cmo myset.cmo dre.cmo ga.cmo naming.cmo name-ga.ml
	ocamlc -g -o $@ /usr/local/lib/ocaml/str.cma $^

name-ga.opt: heap.cmx graph.cmx dijkstra.cmx myset.cmx dre.cmx ga.cmx naming.cmx name-ga.ml
	ocamlopt -o $@ /usr/local/lib/ocaml/str.cmxa $^

ordering-to-tree: heap.cmo graph.cmo dijkstra.cmo myset.cmo dre.cmo naming.cmo linklist.cmo ortc.cmo ordering-to-tree.ml
	ocamlc -g -o $@ /usr/local/lib/ocaml/str.cma $^

ordering-to-tree.opt: heap.cmx graph.cmx dijkstra.cmx myset.cmx dre.cmx naming.cmx linklist.cmx ortc.cmx ordering-to-tree.ml
	ocamlopt -o $@ /usr/local/lib/ocaml/str.cmxa $^

graph-to-tree: heap.cmo graph.cmo dijkstra.cmo myset.cmo dre.cmo ortc.cmo \
	mintree.cmo graph-to-tree.ml
	ocamlc -g -o $@ /usr/local/lib/ocaml/str.cma $^

graph-to-tree.opt: heap.cmx graph.cmx dijkstra.cmx myset.cmx dre.cmx ortc.cmx \
	mintree.cmx graph-to-tree.ml
	ocamlopt -o $@ /usr/local/lib/ocaml/str.cmxa $^

tree-assign: heap.cmo graph.cmo tree-assign.ml
	ocamlc -g -o $@ /usr/local/lib/ocaml/str.cma $^

tree-assign.opt: heap.cmx graph.cmx tree-assign.ml
	ocamlopt -o $@ /usr/local/lib/ocaml/str.cmxa $^

clean:
	-rm *.cmo *.cmx *.cmi graph2dot test-heap test-dijk test-dijk.opt
