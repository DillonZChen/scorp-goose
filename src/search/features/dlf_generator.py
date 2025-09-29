import sys
from contextlib import contextmanager
import os
from dlplan.core import VocabularyInfo, InstanceInfo, State, SyntacticElementFactory
from dlplan.generator import generate_features

Fact = tuple[str, list[str]]  # (predicate, [arg1, arg2, ...])


class DLFGenerator:
    def __init__(
        self, predicates: set[tuple[str, int]], facts: list[Fact], goals: list[Fact]
    ):
        self.vocabulary = VocabularyInfo()
        for predicate, arity in predicates:
            self.vocabulary.add_predicate(predicate, arity)
            self.vocabulary.add_predicate(predicate + "__g", arity)

        self.instance = InstanceInfo(0, self.vocabulary)
        for predicate, args in facts:
            self.instance.add_atom(predicate, args)
        for predicate, args in goals:
            self.instance.add_static_atom(predicate + "__g", args)

        self.factory = SyntacticElementFactory(self.vocabulary)
        self.atoms = self.instance.get_atoms()

        self._features = {}

    def get_num_features(self) -> int:
        return len(self._features)

    @contextmanager
    def _suppress_os_output(self):
        """
        Context manager to suppress C/C++ output by redirecting
        OS-level file descriptors (1 for stdout, 2 for stderr).
        This works on Unix-like systems (Linux/macOS) and should
        handle output from C++ extension modules.
        """
        if os.name != "posix":
            # Fallback for non-POSIX systems, though less effective for C++ output
            from io import StringIO

            original_stdout = sys.stdout
            original_stderr = sys.stderr
            sys.stdout = StringIO()
            sys.stderr = StringIO()
            try:
                yield
            finally:
                sys.stdout = original_stdout
                sys.stderr = original_stderr
            return

        # POSIX (Linux/macOS) system redirection
        # 1. Save the original FDs for stdout (1) and stderr (2)
        devnull_fd = os.open(os.devnull, os.O_WRONLY)
        original_stdout_fd = os.dup(1)
        original_stderr_fd = os.dup(2)

        # 2. Redirect stdout (1) and stderr (2) to /dev/null
        os.dup2(devnull_fd, 1)
        os.dup2(devnull_fd, 2)

        try:
            yield
        finally:
            # 3. Restore the original FDs
            os.dup2(original_stdout_fd, 1)
            os.dup2(original_stderr_fd, 2)

            # 4. Close temporary FDs
            os.close(devnull_fd)
            os.close(original_stdout_fd)
            os.close(original_stderr_fd)

    def generate_features(self, facts: list[int]) -> list[tuple[int, int]]:
        features = []
        state = State(0, self.instance, [self.atoms[i] for i in facts])

        with self._suppress_os_output():  # hide dlplan logging
            (
                generated_booleans,
                generated_numericals,
                generated_concepts,
                generated_roles,
            ) = generate_features(
                factory=self.factory,
                states=[state],
                concept_complexity_limit=5,
                role_complexity_limit=5,
                boolean_complexity_limit=10,
                count_numerical_complexity_limit=10,
                distance_numerical_complexity_limit=0,
            )

        for f in generated_booleans:
            if f not in self._features:
                self._features[f] = len(self._features)
            features.append((self._features[f], int(f.evaluate(state))))

        for f in generated_numericals:
            if f not in self._features:
                self._features[f] = len(self._features)
            features.append((self._features[f], int(f.evaluate(state))))

        return features
