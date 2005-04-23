all: test-heap graph2dot test-dijk test-dijk.opt test-dre test-dre.opt

%.cmx: %.ml
	ocamlopt -c $<

%.cmo: %.ml $(if -e %.mli,%.cmi)
	ocamlc -g -c $<

%.cmo: %.ml
	ocamlc -g -c $<

%.cmi: %.mli
	ocamlc -g -c $<

graph2dot: graph2dot.ml graph.cmo
	ocamlc -g -o graph2dot graph.cmo str.cma graph2dot.ml

test-heap: test-heap.ml heap.cmo heap.cmi
	ocamlc -g -o test-heap heap.cmo test-heap.ml

test-dijk: test-dijk.ml heap.cmo heap.cmi graph.cmo dijkstra.cmo
	ocamlc -g -o test-dijk heap.cmo graph.cmo dijkstra.cmo /usr/local/lib/ocaml/str.cma test-dijk.ml

test-dijk.opt: heap.cmx graph.cmx dijkstra.cmx test-dijk.ml
	ocamlopt -o $@ /usr/local/lib/ocaml/str.cmxa $^

test-dre: heap.cmo graph.cmo dijkstra.cmo dre.cmo test-dre.ml
	ocamlc -g -o $@ /usr/local/lib/ocaml/str.cma $^

test-dre.opt: heap.cmx graph.cmx dijkstra.cmx dre.cmx test-dre.ml
	ocamlopt -o $@ /usr/local/lib/ocaml/str.cmxa $^

clean:
	-rm *.cmo *.cmx *.cmi graph2dot test-heap test-dijk test-dijk.opt
